/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "FirewallClient.h"

#include <array>
#include <memory>
#include <stdexcept>
#include <mbedtls/x509_crt.h>
#include "http/Request.h"
#include "http/Cookie.h"
#include "http/Cookies.h"
#include "http/Url.h"
#include "tools/Json11.h"
#include "tools/Logger.h"
#include "tools/pugixml.hpp"
#include "tools/StringMap.h"
#include "tools/StrUtil.h"
#include "tools/X509Crt.h"


namespace fw {

	using namespace tools;

	FirewallClient::FirewallClient(const net::Endpoint& ep, const std::string& realm, const net::TlsConfig& config):
		HttpsClient(ep, config),
		_peer_crt_digest(),
		_cookie_jar(),
		_mutex(),
		_realm(realm)
	{
		DEBUG_CTOR(_logger);

		set_hostname_verification(true);
	}


	FirewallClient::~FirewallClient()
	{
		DEBUG_DTOR(_logger);
	}


	portal_err FirewallClient::open(const confirm_crt_fn& confirm_crt)
	{
		DEBUG_ENTER(_logger);
		Mutex::Lock lock{ _mutex };
		http::Answer answer;

		_logger->info(">> connecting to %s", host().to_string().c_str());

		try {
			HttpsClient::connect();
		}
		catch (const std::runtime_error& e) {
			_logger->error("ERROR: failed to connect to %s", host().to_string().c_str());
			_logger->error("ERROR: %s", e.what());

			return portal_err::COMM_ERROR;
		}

		_logger->info(">> protocol : %s", get_tls_version().c_str());
		_logger->info(">> cipher : %s", get_ciphersuite().c_str());

		/*
			Checks whether the server certificate is trusted.

			The certificate is first validated against the CA configured during
			mbedtls initialization. The get_crt_check() call returns the result
			of that validation.

			If the certificate is not trusted (e.g., the CA is unknown), we then
			attempt to validate it using the CAs stored in the Windows root
			certificate store.
		*/
		int crt_status = get_crt_check();

		if (crt_status & MBEDTLS_X509_BADCERT_NOT_TRUSTED) {
			if (x509crt_is_trusted(get_peer_crt()))
				crt_status &= ~MBEDTLS_X509_BADCERT_NOT_TRUSTED;
		}

		if (crt_status == 0) {
			_logger->info(">> peer X.509 certificate valid");

		}
		else {
			_logger->info(">> peer X.509 certificate error");

			if (_logger->is_debug_enabled()) {
				std::array<char, 4096> buffer = { 0 };
				mbedtls_x509_crt_info(buffer.data(), buffer.size() - 1, "   ", get_peer_crt());
				_logger->debug(buffer.data());
			}

			if (!confirm_crt(get_peer_crt(), crt_status)) {
				return portal_err::CERT_UNTRUSTED;
			}
		}

		/*
			Compute the digest of the certificate.
			This digest is checked during a future reconnection to verify the certificate.
		*/
		_peer_crt_digest = CrtDigest(get_peer_crt());


		/*
			Fetch the top page, following up to two redirects if necessary.
			Then verify that the resulting top page exists.
		*/
		if (!send_request(http::Request::GET_VERB, make_url("/" + _realm), "", http::Headers(), answer, 2))
			return portal_err::COMM_ERROR;

		if (answer.get_status_code() != HttpsClient::STATUS_OK) {
			log_http_error("firewall portal connection failure", answer);
			return portal_err::HTTP_ERROR;
		}

		return portal_err::NONE;
	}


	void FirewallClient::log_http_error(const char* msg, const http::Answer& answer)
	{
		_logger->error("ERROR: %s", msg);
		_logger->error(
			"ERROR: %s (HTTP code %d)",
			answer.get_reason_phrase().c_str(),
			answer.get_status_code()
		);
	}


	portal_err FirewallClient::try_login_check(const tools::StringMap& in_params, tools::StringMap& out_params)
	{
		DEBUG_ENTER(_logger);

		http::Answer answer;
		http::Headers headers;
		headers.set("Content-Type", "text/plain;charset=UTF-8");

		if (!send_request(http::Request::POST_VERB, make_url("/remote/logincheck"), in_params.join("&"), headers, answer, 0))
			return portal_err::COMM_ERROR;

		if (!(answer.get_status_code() == HttpsClient::STATUS_OK ||
			answer.get_status_code() == HttpsClient::STATUS_UNAUTHORIZED ||
			answer.get_status_code() == HttpsClient::STATUS_FORBIDDEN)) {
			log_http_error("firewall portal connection failure", answer);
			return portal_err::HTTP_ERROR;
		}

		/*
			The response body contains the result of the login check call.
			It is formatted as a list of comma-separated key/value pairs, for example:

				ret=6,magic=123,actionurl=/remote/logincheck,reqid=456,portal=,
				grpid=0,pid=214,is_chal_rsp=1,chal_msg=Enter your code

			We parse these comma-separated parameters from the firewall response
			and populate a string map that will later be used to retrieve them.  We
			also verify that the output `ret` parameter exists.
		*/
		out_params.serase();
		out_params.add(std::string(answer.body().cbegin(), answer.body().cend()), ',');

		int retcode;
		if (!out_params.get_int("ret", retcode)) {
			_logger->error("ERROR: invalid firewall answer, ret code missing");
			return portal_err::ACCESS_DENIED;
		}

		return portal_err::NONE;
	}


	portal_err FirewallClient::login_basic(const ask_credentials_fn& ask_credential, const ask_pincode_fn& ask_code)
	{
		DEBUG_ENTER(_logger);
		tools::Mutex::Lock lock{ _mutex };

		// Misc initializations.
		http::Answer answer;
		tools::StringMap params_query;
		AuthCredentials credentials;

		/*
			Fetch the login page. This operation obtains the cookies
			that the FortiGate firewall uses for the authentication process.
		*/
		if (!_realm.empty())
			params_query.set("realm", _realm);
		params_query.set("lang", "en");
		const http::Url login_url = make_url("/remote/login", params_query.join("&"));
		if (!send_request(http::Request::GET_VERB, login_url, "", http::Headers(), answer, 0))
			return portal_err::COMM_ERROR;

		if (answer.get_status_code() != HttpsClient::STATUS_OK) {
			log_http_error("firewall portal connection failure", answer);
			return portal_err::HTTP_ERROR;
		}

		/*
			Check whether the client is already authenticated.
			In theory, the user should not be authenticated at this point.
			This check could be removed, but it is kept as a safeguard in case
			the main window logic fails to correctly manage the login button state.
		*/
		if (is_authenticated()) {
			return portal_err::NONE;
		}

		// Show login prompt and ask credentials.
		_logger->info(">> auth mode : basic");
		if (!ask_credential(credentials))
			return portal_err::LOGIN_CANCELLED;

		// Prepare the HTML login form.
		params_query.serase();
		params_query.set("ajax", "1");
		params_query.set("username", HttpsClient::encode_url(credentials.username));
		if (!_realm.empty())
			params_query.set("realm", _realm);
		params_query.set("credential", HttpsClient::encode_url(credentials.password));

		// Safe erase password stored in memory.
		tools::serase(credentials.password);

		// Loop until code returns with access denied, login canceled or an error
		// is detected.
		while (true) {
			tools::StringMap params_result;
			portal_err err = try_login_check(params_query, params_result);
			if (err != portal_err::NONE)
				return err;

			// Get the result of the last authentication check.
			int retcode = params_result.get_int_value("ret", -1);

			if (retcode == 0) {
				// ********************************
				// 0 = Access denied.
				// ********************************

				// Show the error message received from the firewall.
				http::Url redir_url;
				if (!get_redir_url(params_result, redir_url)) {
					_logger->error("ERROR: invalid firewall answer, redir missing");
				}

				std::string msg = "access denied";
				redir_url.get_query_map().get_str("err", msg);
				_logger->error("ERROR: %s", msg.c_str());

				return portal_err::ACCESS_DENIED;
			}

			if (retcode == 1) {
				// ********************************
				// 1 = Access allowed.
				// ********************************

				// Follow the redirect URL to get a valid SVPNCOOKIE.
				// The SVPNCOOKIE is later transmitted to the firewall when starting
				// the tunnel mode.
				http::Url redir_url;
				if (!get_redir_url(params_result, redir_url)) {
					_logger->error("ERROR: invalid firewall answer, redir missing");
				}

				redir_url = make_url(redir_url.get_path(), redir_url.get_query());
				if (!send_request(http::Request::GET_VERB, redir_url, "", http::Headers(), answer, 0))
					return portal_err::COMM_ERROR;

				return portal_err::NONE;
			}

			switch (retcode) {
			case 2:
			case 3:
			case 4: {
				// ********************************
				// 2 = Fortitoken (** Never tested **)
				// 3 = Email based two-factor authentication
				// 4 = SMS based two-factor authentication
				// ********************************
				static const std::array<std::string, 3> messages = {
					/*2: */ "Enter fortitoken code ",
					/*3: */ "Enter authentication code sent to email ",
					/*4: */ "Enter authentication code sent to SMS "
				};

				AuthCode code;
				std::string device;

				if (params_result.get_str("tokeninfo", device)) {
					device = http::HttpsClient::decode_url(device);
					code.prompt = messages[retcode - 2] + device;
				}
				else {
					code.prompt = "Enter authentication code";
				}
				code.code = "";
				if (!ask_code(code))
					return portal_err::LOGIN_CANCELLED;

				params_query.set("code", code.code);
				params_query.set("code2", "");
				params_query.set("reqid", params_result.get_str_value("reqid", ""));
				params_query.set("polid", params_result.get_str_value("polid", ""));
				params_query.set("grp", params_result.get_str_value("grp", ""));
			}
			break;

			case 5: {
				// ********************************
				// 5: FotiToken drifted, require next code
				//    ** Never tested **
				// ********************************
				AuthCode code{ "Wait next code", "" };
				if (!ask_code(code))
					return portal_err::LOGIN_CANCELLED;

				params_query.set("code", "");
				params_query.set("code2", code.code);
				params_query.set("reqid", params_result.get_str_value("reqid", ""));
				params_query.set("polid", params_result.get_str_value("polid", ""));
				params_query.set("grp", params_result.get_str_value("grp", ""));
			}
			break;

			case 6: {
				// ********************************
				// 6: Challenge
				// ********************************
				if (params_result.get_int_value("pass_renew", 0) == 1) {
					// password renewal not supported.
					_logger->error("ERROR: password expired");
					return portal_err::LOGIN_CANCELLED;
				}

				// there is a challenge message
				AuthCode challenge;

				// .. assign a default prompt
				challenge.prompt = params_result.get_str_value("chal_msg", "enter code");

				// .. ask a code to the user
				if (!ask_code(challenge))
					return portal_err::LOGIN_CANCELLED;

				// .. build the send_request
				params_query.set("magic", params_result.get_str_value("magic", ""));
				params_query.set(
					"reqid",
					tools::string_format(
						"%s,%s",
						params_result.get_str_value("reqid", "").c_str(),
						params_result.get_str_value("polid", "").c_str()
					));
				params_query.set(
					"grpid",
					tools::string_format(
						"%s,%s,%s",
						params_result.get_str_value("grpid", "").c_str(),
						params_result.get_str_value("pid", "").c_str(),
						params_result.get_str_value("is_chal_rsp", "").c_str()
					));
				params_query.set("credential2", challenge.code);
			}
			break;

			default:
				_logger->error("ERROR: unknown return code %d during authentication", retcode);
				return portal_err::ACCESS_DENIED;
			}

			std::string peer;
			if (params_result.get_str("peer", peer))
				params_query.set("peer", peer);

			// loop and retry login
		}
	}


	portal_err FirewallClient::login_saml(const ask_samlauth_fn& ask_samlauth)
	{
		DEBUG_ENTER(_logger);
		tools::Mutex::Lock lock{ _mutex };

		std::string service_provider_crt;
		if (!X509crt_to_pem(get_peer_crt(), service_provider_crt))
			return portal_err::CERT_INVALID;

		AuthSamlInfo saml_auth_info{
			make_url("/remote/saml/start", "realm=" + _realm),
			service_provider_crt,
			_cookie_jar,
			[this]() -> bool { return this->is_authenticated(); }
		};

		_logger->info(">> auth mode : saml");
		if (!ask_samlauth(saml_auth_info))
			goto terminate;

		return portal_err::NONE;

	terminate:
		return portal_err::LOGIN_CANCELLED;
	}


	bool FirewallClient::logout()
	{
		DEBUG_ENTER(_logger);
		tools::Mutex::Lock lock{ _mutex };

		http::Headers headers;
		http::Answer answer;

		const http::Url logout_url = make_url("/remote/logout");
		bool ok = send_request(http::Request::GET_VERB, logout_url, "", headers, answer, 0);

		// Delete all session cookies
		if (_logger->is_trace_enabled()) {
			_logger->trace("... 0x%012Ix clear cookie jar", PTR_VAL(this));
		}
		_cookie_jar.clear();

		return ok;
	}


	bool FirewallClient::get_info(PortalInfo& portal_info)
	{
		DEBUG_ENTER(_logger);
		tools::Mutex::Lock lock{ _mutex };

		if (!is_authenticated())
			return false;

		http::Headers headers;
		http::Answer answer;

		const http::Url portal_url = make_url("/remote/portal", "access");
		if (!send_request(http::Request::GET_VERB, portal_url, "", headers, answer, 0))
		{
			_logger->error("ERROR: get portal info failure");
			return false;
		}

		if (answer.get_status_code() != HttpsClient::STATUS_OK) {
			log_http_error("get portal info failure ", answer);
			return false;
		}
	
		const tools::ByteBuffer& body = answer.body();
		const std::string data{ body.cbegin(), body.cend() };
		_logger->debug("... portal_info : %s...", data.substr(0, 80).c_str());

		std::string parser_error;
		tools::Json info = tools::Json::parse(data, parser_error);
		if (info.is_object()) {
			tools::Json user = info["user"];
			tools::Json group = info["group"];
			tools::Json version = info["version"];

			portal_info.user = user.is_string() ? user.string_value() : "";
			portal_info.group = group.is_string() ? group.string_value() : "";
			portal_info.version = version.is_string() ? version.string_value() : "";
		}

		return true;
	}


	bool FirewallClient::get_config(SslvpnConfig& sslvpn_config)
	{
		DEBUG_ENTER(_logger);
		tools::Mutex::Lock lock{ _mutex };

		if (!is_authenticated())
			return false;

		http::Headers headers;
		http::Answer answer;

		/*
			Retrieve the SSL VPN configuration.
			This call is mandatory because the FortiGate will not provide an IP address otherwise.
		*/
		const http::Url vpninfo_url = make_url("/remote/fortisslvpn_xml");
		if (!send_request(http::Request::GET_VERB, vpninfo_url, "", headers, answer, 0))
		{
			_logger->error("ERROR: get portal configuration failure");
			return false;
		}

		if (answer.get_status_code() != HttpsClient::STATUS_OK) {
			log_http_error("get portal configuration failure ", answer);
			return false;
		}

		const tools::ByteBuffer& body = answer.body();
		const std::string data(body.cbegin(), body.cend());

		pugi::xml_document doc;
		pugi::xml_parse_result parse_result = doc.load_string(data.c_str());

		if (parse_result.status != pugi::xml_parse_status::status_ok) {
			_logger->error("ERROR: portal configuration - XML parse error");
			return false;
		}

		const pugi::xml_node& root = doc.child("sslvpn-tunnel");
		if (root.empty()) {
			_logger->error("ERROR: portal configuration - XML decode error");
			return false;
		}

		const pugi::xml_attribute& address = root
			.child("ipv4")
			.child("assigned-addr")
			.attribute("ipv4");
		sslvpn_config.local_addr = address.as_string();

		return true;
	}


	bool FirewallClient::is_authenticated() const
	{
		if (!_cookie_jar.exists("SVPNCOOKIE"))
			return false;

		const http::Cookie& svpn_cookie = _cookie_jar.get("SVPNCOOKIE");
		return svpn_cookie.get_value().size() > 0 && !svpn_cookie.is_expired();
	}


	fw::FirewallTunnel* FirewallClient::create_tunnel(const net::Endpoint& local_ep,
		const net::Endpoint& remote_ep, const net::tunneler_config& config)
	{
		DEBUG_ENTER(_logger);

		return new fw::FirewallTunnel(
			std::make_unique<http::HttpsClient>(host(), get_tls_config()),
			local_ep,
			remote_ep,
			config,
			_cookie_jar
		);
	}


	bool FirewallClient::send_and_receive(http::Request& request, http::Answer& answer)
	{
		DEBUG_ENTER(_logger);

		if (is_reconnection_required()) {
			disconnect();

			try {
				connect();
			}
			catch (std::runtime_error& e) {
				_logger->error("ERROR: failed to connect to %s", host().to_string().c_str());
				_logger->error("ERROR: %s", e.what());

				return false;
			}

			/*
				We verify the certificate using the previously computed digest rather than
				revalidating the entire certificate chain on every connection.

				Re-validating the full certificate chain is unnecessary and inefficient once the
				server identity has already been established. Certificate chains typically do not
				change during a session or across short reconnections, and performing a full
				validation repeatedly would add significant overhead—especially when using
				external trust stores or when network conditions introduce latency.

				By storing the digest of the server certificate after the first validated
				connection, we can quickly confirm that the server is presenting the same
				certificate on subsequent reconnects. This provides the necessary security check
				(detecting any certificate or man-in-the-middle changes) while avoiding redundant
				and costly full-chain validation operations.
			*/
			if (_peer_crt_digest != CrtDigest(get_peer_crt())) {
				_logger->error("ERROR: invalid certificate digest");

				return false;
			}
		}

		try {
			HttpsClient::send_request(request);
		}
		catch (std::runtime_error& e) {
			_logger->error("ERROR: failed to send HTTP request to %s", host().to_string().c_str());
			_logger->error("ERROR: %s", e.what());

			return false;
		}

		try {
			recv_answer(answer);
		}
		catch (std::runtime_error& e) {
			_logger->error("ERROR: failed to receive HTTP data from %s", host().to_string().c_str());
			_logger->error("ERROR: %s", e.what());

			return false;
		}
		
		return true;
	}


	bool FirewallClient::do_request(const std::string& verb, const http::Url& url,
		const std::string& body, const http::Headers& headers, http::Answer& answer)
	{
		DEBUG_ENTER(_logger);
		
		http::Request request{ verb, url, _cookie_jar };

		request.headers()
			.set("Accept", "text/html")
			.set("Accept-Encoding", "identity")
			.set("Accept-Language", "en")
			.set("Cache-Control", "no-cache")
			.set("Connection", "keep-alive")
			.set("Host", host().to_string())
			.set("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64)")
			.add(headers);

		request.set_body(reinterpret_cast<const unsigned char*>(body.c_str()), body.length());

		// Send and wait for a response
		const bool success = FirewallClient::send_and_receive(request, answer);
		if (!success) {
			disconnect();
		}
		else {
			/*
				Process cookies received in the HTTP response and update the local cookie jar.

				For each cookie whose domain matches the send_request URL (or is unspecified):
				  - Remove it if it has expired.
				  - Add it if it is marked as Secure and HttpOnly.
				  - Otherwise, ignore it.

				This ensures that only valid, secure cookies for the target domain are stored,
				while expired or irrelevant cookies are removed or skipped.
			*/
			for (auto it = answer.cookies().cbegin(); it != answer.cookies().cend(); it++) {
				const http::Cookie& cookie = it->second;
				const std::string url_domain = url.get_hostname();

				if (cookie.get_domain().empty() || cookie.same_domain(url_domain)) {
					if (cookie.is_expired()) {
						_logger->debug("... 0x%012Ix       remove expired cookie name=%s expires=%d from cookiejar", 
								PTR_VAL(this),
								cookie.get_name().c_str(),
								cookie.get_expires());

						_cookie_jar.remove(cookie.get_name());
					}
					else if (cookie.is_secure() && cookie.is_http_only()) {
							_logger->debug("... 0x%012Ix       add cookie name=%s expires=%d to cookiejar",
								PTR_VAL(this),
								cookie.get_name().c_str(),
								cookie.get_expires());

						_cookie_jar.add(http::Cookie(
							cookie.get_name(),
							cookie.get_value(),
							url_domain,
							cookie.get_path(),
							cookie.get_expires(),
							true,
							true
						));
					}
					else {
						_logger->debug("... 0x%012Ix       skip cookie %s",
							PTR_VAL(this),
							cookie.get_name().c_str());
					}
				}
			}
		}

		_logger->debug("... 0x%012Ix       %s::%s : %s %s (status=%s (%d))",
			PTR_VAL(this),
			__class__,
			__func__,
			verb.c_str(),
			url.to_string(false).c_str(),
			answer.get_reason_phrase().c_str(),
			answer.get_status_code());

		return success;
	}


	bool FirewallClient::send_request(const std::string& verb, const http::Url& url,
		const std::string& body, const http::Headers& headers, http::Answer& answer, int allow_redir)
	{
		if (allow_redir < 0) {
			_logger->error("ERROR: Redirect failed");
			return false;
		}

		if (!do_request(verb, url, body, headers, answer))
			return false;

		// Redirect if required
		const int status_code = answer.get_status_code();
		if ((status_code == HttpsClient::STATUS_TEMPORARY_REDIRECT ||
			status_code == HttpsClient::STATUS_FOUND ||
			status_code == HttpsClient::STATUS_SEE_OTHER) && allow_redir >= 0) {
			std::string location;
			if (answer.headers().get("Location", location)) {
				const http::Url redir_url{ location };
				if (!send_request(verb, make_url(redir_url.get_path(), redir_url.get_query()), body, headers, answer, allow_redir - 1))
					return false;
			}
		}

		return true;
	}


	bool FirewallClient::get_redir_url(const tools::StringMap& params, http::Url& url) const
	{
		std::string redir;
		if (!params.get_str("redir", redir))
			return false;
		
		url = http::Url(decode_url(redir));
		return true;
	}

	const char* FirewallClient::__class__ = "FirewallClient";
}
