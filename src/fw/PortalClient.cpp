/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "PortalClient.h"

#include <mbedtls/x509_crt.h>
#include "http/Request.h"
#include "http/Cookie.h"
#include "http/Cookies.h"
#include "http/Url.h"
#include "tools/ErrUtil.h"
#include "tools/Json11.h"
#include "tools/Logger.h"
#include "tools/pugixml.hpp"
#include "tools/StringMap.h"
#include "tools/StringMap.h"
#include "tools/StrUtil.h"
#include "tools/X509Crt.h"


namespace fw {

	using namespace tools;

	PortalClient::PortalClient(const net::Endpoint& ep, const std::string& realm):
		HttpsClient(ep),
		_peer_crt_digest(),
		_mutex(),
		_realm(realm),
		_cookie_jar()
	{
		DEBUG_CTOR(_logger, "PortalClient");

		// Configure the tls client.
		if (!set_timeout(10000, 10000))
			_logger->error("ERROR: failed to configure timeout parameters");
		set_hostname_verification(true);
	}


	PortalClient::~PortalClient()
	{
		DEBUG_DTOR(_logger, "PortalClient");

	}


	portal_err PortalClient::open(confirm_crt_fn confirm_crt)
	{
		DEBUG_ENTER(_logger, "PortalClient", "open");
		Mutex::Lock lock{ _mutex };
		http::Answer answer;

		_logger->info(">> connecting to %s", host().to_string().c_str());

		// Connect to the server
		try {
			HttpsClient::connect();
		}
		catch (const mbed_error& e) {
			_logger->error("ERROR: failed to connect to %s", host().to_string().c_str());
			_logger->error("ERROR: %s", e.message().c_str());

			return portal_err::COMM_ERROR;
		}

		_logger->info(">> protocol : %s", get_tls_version().c_str());
		_logger->info(">> cipher : %s", get_ciphersuite().c_str());

		// Check the server certificate
		int crt_status = get_crt_check();

		if (crt_status & MBEDTLS_X509_BADCERT_NOT_TRUSTED) {
			// The root certificate is not stored in the local certificate files.
			// Check if it is stored in Windows CA root store.
			if (x509crt_is_trusted(get_peer_crt()))
				crt_status &= ~MBEDTLS_X509_BADCERT_NOT_TRUSTED;
		}

		if (crt_status == 0) {
			_logger->info(">> peer X.509 certificate valid");

		}
		else {
			_logger->info(">> peer X.509 certificate error");

			if (_logger->is_debug_enabled()) {
				char buffer[4096] = { 0 };
				mbedtls_x509_crt_info(buffer, sizeof(buffer) - 1, "   ", get_peer_crt());
				_logger->debug(buffer);
			}

			if (!confirm_crt(get_peer_crt(), crt_status)) {
				return portal_err::CERT_UNTRUSTED;
			}
		}

		// Compute the digest of the certificate.
		// The digest is validated later when we reconnect to the server.
		_peer_crt_digest = CrtDigest(get_peer_crt());

		// Get the top page.
		// Allows redirection if required.
		if (!request(http::Request::GET_VERB, make_url("/" + _realm), "", http::Headers(), answer, 2))
			return portal_err::COMM_ERROR;

		// Check if the web page exists.
		if (answer.get_status_code() != HttpsClient::STATUS_OK) {
			_logger->error("ERROR: portal connection failure");
			_logger->error("ERROR: %s (HTTP code %d)",
					answer.get_reason_phrase().c_str(),
					answer.get_status_code()
				);

			return portal_err::HTTP_ERROR;
		}

		return portal_err::NONE;
	}


	bool PortalClient::login_check(const tools::StringMap& params, http::Answer& answer)
	{
		DEBUG_ENTER(_logger, "PortalClient", "login_check");

		http::Headers headers;
		headers.set("Content-Type", "text/plain;charset=UTF-8");
		
		return request(http::Request::POST_VERB, make_url("/remote/logincheck"), params.join("&"), headers, answer, 0);
	}


	portal_err PortalClient::login_basic(ask_credentials_fn ask_credential, ask_pincode_fn ask_code)
	{
		DEBUG_ENTER(_logger, "PortalClient", "login_basic");
		tools::Mutex::Lock lock{ _mutex };

		// misc initializations
		int retcode = 0;
		tools::StringMap params_query;
		tools::StringMap params_result;
		http::Answer answer;
		std::string body;
		AuthCredentials credentials;


		// Get the login form
		if (!_realm.empty())
			params_query.set("realm", _realm);
		params_query.set("lang", "en");
		const http::Url login_url = make_url("/remote/login", params_query.join("&"));
		if (!request(http::Request::GET_VERB, login_url, "", http::Headers(), answer, 0))
			goto comm_error;

		// check if page is available
		if (answer.get_status_code() != HttpsClient::STATUS_OK)
			goto http_error;

		// check if the client is authenticated.
		if (authenticated()) {
			return portal_err::NONE;
		}

		// Ask credentials.
		_logger->info(">> auth mode : basic");
		if (!ask_credential(credentials))
			goto terminate;

		// Prepare the login form.
		params_query.serase();
		if (!_realm.empty())
			params_query.set("realm", _realm);
		params_query.set("ajax", "1");
		params_query.set("username", HttpsClient::url_encode(credentials.username));
		params_query.set("credential", HttpsClient::url_encode(credentials.password));

		// Safe erase password stored in memory.
		tools::serase(credentials.password);

		// Send the login form.
		if (!login_check(params_query, answer))
			goto comm_error;

		if (!(answer.get_status_code() == HttpsClient::STATUS_OK ||
			answer.get_status_code() == HttpsClient::STATUS_UNAUTHORIZED ||
			answer.get_status_code() == HttpsClient::STATUS_FORBIDDEN))
			goto http_error;

		// Get the body from the answer.
		body = std::string(answer.body().cbegin(), answer.body().cend());

		// Check if the return code is present.
		params_result.add(body, ',');
		if (params_result.get_int("ret", retcode)) {
			// firewall is running FortiOS 4 or higher

			// Loop until authenticated. The loop breaks if an error (access denied,
			// communication error) is detected.
			while (!authenticated()) {
				switch (retcode) {
				case 0: {
					// Access denied,
					// show the error message received from the firewall
					http::Url redir_url;
					if (!get_redir_url(params_result, redir_url))
						goto redir_error;

					std::string msg;
					if (redir_url.get_query_map().get_str("err", msg)) {
						_logger->error("ERROR: %s", msg.c_str());
					}
				}
				goto auth_error;
				break;

				case 1: {
					// Access allowed
					http::Url redir_url;
					if (!get_redir_url(params_result, redir_url))
						goto redir_error;

					redir_url = make_url(redir_url.get_path(), redir_url.get_query());
					if (!request(http::Request::GET_VERB, redir_url, "", http::Headers(), answer, 0))
						goto comm_error;

					if (answer.get_status_code() != HttpsClient::STATUS_OK)
						goto http_error;
				}
				break;

				case 3:
				case 4: {
					// Ask for a token value
					// 3 = Email based two-factor authentication
					// 4 = SMS based two-factor authentication
					AuthCode code2fa;
					std::string device;
					if (params_result.get_str("tokeninfo", device)) {
						device = http::HttpsClient::url_decode(device);
						code2fa.prompt = "Authentication code for " + device;
					}
					else {
						code2fa.prompt = "Authentication code";
					}
					code2fa.code = "";
					ask_code(code2fa);
					std::string data;

					params_query.set("code", code2fa.code);
					params_query.set("code2", "");
					params_query.set("realm", params_result.get_str("realm", data) ? data : "");
					params_query.set("reqid", params_result.get_str("reqid", data) ? data : "");
					params_query.set("polid", params_result.get_str("polid", data) ? data : "");
					params_query.set("grp", params_result.get_str("grp", data) ? data : "");

					if (!login_check(params_query, answer))
						goto comm_error;
					if (answer.get_status_code() != HttpsClient::STATUS_OK)
						goto http_error;

					// check if the return code is present
					retcode = 0;
					params_result.serase();
					params_result.add(std::string(answer.body().cbegin(), answer.body().cend()), ',');
					params_result.get_int("ret", retcode);
				}
				break;

				case 6: {
					int pass_renew;

					if (params_result.get_int("pass_renew", pass_renew) && pass_renew == 1) {
						_logger->error("ERROR: password expired");
						goto auth_error;

					}
					else {
						// there is a challenge message 
						AuthCode challenge;
						
						// .. assign a default prompt
						challenge.prompt = "enter code";
						params_result.get_str("chal_msg", challenge.prompt);

						// .. ask a code the user
						ask_code(challenge);
						
						// .. build the request
						std::string data;
						params_query.set("realm", params_result.get_str("realm", data) ? data : "");
						params_query.set("magic", params_result.get_str("magic", data) ? data : "");
						params_query.set(
							"reqid",
							tools::string_format(
								"%s,%s,%s", 
								(params_result.get_str("reqid", data) ? data : "").c_str(),
								(params_result.get_str("polid", data) ? data : "").c_str(),
								(params_result.get_str("sp_polid", data) ? data : "").c_str()
							));
						params_query.set(
							"grpid",
							tools::string_format(
								"%s,%s,%s",
								(params_result.get_str("grpid", data) ? data : "").c_str(),
								(params_result.get_str("pid", data) ? data : "").c_str(),
								(params_result.get_str("usr_only_check", data) ? data : "").c_str()
							));
						params_query.set("credential", challenge.code);

						if (!login_check(params_query, answer))
							goto comm_error;
						if (answer.get_status_code() != HttpsClient::STATUS_OK)
							goto http_error;

						// check if the return code is present
						retcode = 0;
						params_result.serase();
						params_result.add(std::string(answer.body().cbegin(), answer.body().cend()), ',');
						params_result.get_int("ret", retcode);
					}
				}
				break;

				default:
					_logger->error("ERROR: unknown return code %d during authentication", retcode);
					goto terminate;
				}
			}
		}
		else {
			_logger->error("ERROR: invalid firewall answer");
			goto terminate;
		}

		return portal_err::NONE;


	comm_error:
		return portal_err::COMM_ERROR;

	http_error:
		_logger->error("ERROR: portal login_basic failure - %s (HTTP code %d)",
			answer.get_reason_phrase().c_str(),
			answer.get_status_code());
		return portal_err::HTTP_ERROR;

	auth_error:
		return portal_err::ACCESS_DENIED;

	redir_error:
		return portal_err::HTTP_ERROR;

	terminate:
		return portal_err::LOGIN_CANCELLED;
	}


	portal_err PortalClient::login_saml(ask_samlauth_fn ask_samlauth)
	{
		DEBUG_ENTER(_logger, "PortalClient", "login_saml");
		tools::Mutex::Lock lock{ _mutex };

		std::string service_provider_crt;
		if (!X509crt_to_pem(get_peer_crt(), service_provider_crt))
			return portal_err::CERT_INVALID;

		AuthSamlInfo saml_auth_info{
			make_url("/remote/saml/start", "realm=" + _realm).to_string(false),
			service_provider_crt,
			_cookie_jar,
			[this]() -> bool { return this->authenticated(); }
		};

		_logger->info(">> auth mode : saml");
		if (!ask_samlauth(saml_auth_info))
			goto terminate;

		return portal_err::NONE;

	terminate:
		return portal_err::LOGIN_CANCELLED;
	}


	void PortalClient::logoff()
	{
		DEBUG_ENTER(_logger, "PortalClient", "logoff");
		tools::Mutex::Lock lock{ _mutex };

		// Delete all session cookies
		_cookie_jar.clear();
	}


	bool PortalClient::get_info(PortalInfo& portal_info)
	{
		DEBUG_ENTER(_logger, "PortalClient", "get_info");
		tools::Mutex::Lock lock{ _mutex };

		if (!authenticated())
			return false;

		http::Headers headers;
		http::Answer answer;

		const http::Url portal_url = make_url("/remote/portal", "access");
		bool ok = request(http::Request::GET_VERB, portal_url, "", headers, answer, 0);
		if (ok && answer.get_status_code() == HttpsClient::STATUS_OK) {
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
		}

		return ok;
	}


	bool PortalClient::get_config(SslvpnConfig& sslvpn_config)
	{
		DEBUG_ENTER(_logger, "PortalClient", "get_config");
		tools::Mutex::Lock lock{ _mutex };

		if (!authenticated())
			return false;

		http::Headers headers;
		http::Answer answer;

		// Get the vpn configuration.  
		// This call is mandatory, without it we don't get an IP address from the fortigate
		const http::Url vpninfo_url = make_url("/remote/fortisslvpn_xml");
		if (!request(http::Request::GET_VERB, vpninfo_url, "", headers, answer, 0))
		{
			// Log an error message
			_logger->error("ERROR: portal configuration failure");
			return false;
		}

		if (answer.get_status_code() != HttpsClient::STATUS_OK) {
			// Log an error message
			_logger->error("ERROR: portal configuration failure - %s (HTTP code %d)",
				answer.get_reason_phrase().c_str(),
				answer.get_status_code());

			return false;
		}

		const tools::ByteBuffer& body = answer.body();
		const std::string data(body.cbegin(), body.cend());

		pugi::xml_document doc;
		pugi::xml_parse_result parse_result = doc.load_string(data.c_str());

		if (parse_result.status != pugi::xml_parse_status::status_ok)
			goto xml_parse_error;

		const pugi::xml_node& root = doc.child("sslvpn-tunnel");
		if (root.empty())
			goto xml_decode_error;

		const pugi::xml_attribute& address = root
												.child("ipv4")
												.child("assigned-addr")
												.attribute("ipv4");
		sslvpn_config.local_addr = address.as_string();

		return true;


	xml_parse_error:
		_logger->error("ERROR: portal configuration failure - xml parse error");
		return false;

	xml_decode_error:
		_logger->error("ERROR: portal configuration failure - xml decode error");
		return false;
	}


	bool PortalClient::start_tunnel_mode()
	{
		DEBUG_ENTER(_logger, "PortalClient", "start_tunnel_mode");
		Mutex::Lock lock(_mutex);

		const http::Url tunnel_url = make_url("/remote/sslvpn-tunnel");
		http::Request request{ http::Request::GET_VERB, tunnel_url, _cookie_jar };
		request.headers().set("Host", "sslvpn");

		try {
			send_request(request);

		} catch (const mbed_error& e) {
			_logger->error("ERROR: failed to open the tunnel");
			_logger->error("ERROR: %s", e.message().c_str());

			return false;
		}

		return true;
	}

	bool PortalClient::authenticated() const
	{
		return _cookie_jar.exists("SVPNCOOKIE") &&
			_cookie_jar.get("SVPNCOOKIE").get_value().length() > 0;
	}


	net::Tunneler* PortalClient::create_tunnel(
		const net::Endpoint& local, const net::Endpoint& remote, const net::tunneler_config& config)
	{
		DEBUG_ENTER(_logger, "PortalClient", "create_tunnel");

		return new net::Tunneler(*this, local, remote, config);
	}


	bool PortalClient::send_and_receive(http::Request& request, http::Answer& answer)
	{
		DEBUG_ENTER(_logger, "PortalClient", "send_and_receive");

		if (must_reconnect()) {
			disconnect();

			try {
				connect();
			}
			catch (const mbed_error &e) {
				_logger->error("ERROR: failed to connect to %s", host().to_string().c_str());
				_logger->error("ERROR: %s", e.message().c_str());

				return false;
			}

			// verify the digest of the certificate
			if (_peer_crt_digest != CrtDigest(get_peer_crt())) {
				_logger->error("ERROR: invalid certificate digest");

				return false;
			}
		}

		//.. send the request
		try {
			send_request(request);
		}
		catch (const mbed_error &e) {
			_logger->error("ERROR: failed to send HTTP request to %s", host().to_string().c_str());
			_logger->error("ERROR: %s", e.message().c_str());

			return false;
		}

		//.. receive the answer
		try {
			recv_answer(answer);
		}
		catch (const frdp_error &e) {
			_logger->error("ERROR: failed to receive HTTP data from %s", host().to_string().c_str());
			_logger->error("ERROR: %s", e.message().c_str());

			return false;
		}
		
		return true;
	}


	bool PortalClient::do_request(const std::string& verb, const http::Url& url,
		const std::string& body, const http::Headers& headers, http::Answer& answer)
	{
		DEBUG_ENTER(_logger, "PortalClient", "do_request");
		
		// Prepare the request
		http::Request request{ verb, url, _cookie_jar };

		request.headers()
			.set("Accept", "text/html")
			.set("Accept-Encoding", "gzip")
			.set("Accept-Encoding", "identity")
			.set("Accept-Language", "en")
			.set("Cache-Control", "no-cache")
			.set("Connection", "keep-alive")
			.set("Host", host().to_string())
			.set("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64)")
			.add(headers);

		request.set_body((unsigned char *)body.c_str(), body.length());

		// Send and wait for a response
		const bool success = PortalClient::send_and_receive(request, answer);
		if (!success) {
			// disconnect if not yet done
			disconnect();
		}
		else {
			// Copy cookies
			for (auto it = answer.cookies().cbegin(); it != answer.cookies().cend(); it++) {
				const http::Cookie& cookie = it->second;
				const std::string url_domain = url.get_hostname();

				if (cookie.get_domain().empty() || cookie.same_domain(url_domain)) {
					if (cookie.is_expired())
						_cookie_jar.remove(cookie.get_name());
					else if (cookie.is_secure() && cookie.is_http_only())
						_cookie_jar.add(http::Cookie(
							cookie.get_name(),
							cookie.get_value(),
							url_domain,
							cookie.get_path(),
							cookie.get_expires(),
							true,
							true
						));
					else
						// Do not consider this cookie
						;
				}
			}
		}

		_logger->debug("... %x       PortalClient::do_request : %s %s (status=%s (%d))",
			(uintptr_t)this,
			verb.c_str(),
			url.to_string(false).c_str(),
			answer.get_reason_phrase().c_str(),
			answer.get_status_code());

		return success;
	}


	bool PortalClient::request(const std::string& verb, const http::Url& url,
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
		if ((status_code == STATUS_TEMPORARY_REDIRECT ||
			status_code == HttpsClient::STATUS_FOUND ||
			status_code == STATUS_SEE_OTHER) && allow_redir >= 0) {
			std::string location;
			if (answer.headers().get("Location", location)) {
				const http::Url redir_url{ location };
				if (!request(verb, make_url(redir_url.get_path(), redir_url.get_query()), body, headers, answer, allow_redir - 1))
					return false;
			}
		}

		return true;
	}


	bool PortalClient::get_redir_url(const tools::StringMap& params, http::Url& url) const
	{
		std::string redir;
		if (!params.get_str("redir", redir))
			return false;
		
		url = url_decode(redir);
		return true;
	}

}
