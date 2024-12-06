/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "PortalClient.h"

#include <mbedtls/x509_crt.h>
#include <mbedtls/pk.h>
#include "http/Request.h"
#include "http/Cookie.h"
#include "http/Cookies.h"
#include "http/Url.h"
#include "tools/ErrUtil.h"
#include "tools/Json11.h"
#include "tools/Logger.h"
#include "tools/pugixml.hpp"
#include "tools/StringMap.h"
#include "tools/StrUtil.h"
#include "tools/SysUtil.h"


namespace fw {

	using namespace tools;

	PortalClient::PortalClient(const net::Endpoint& ep, const CertFiles& cert_files):
		HttpsClient(ep),
		_connected(false),
		_peer_crt_digest(),
		_mutex(),
		_cert_files(cert_files),
		_cookies()
	{
		DEBUG_CTOR(_logger, "PortalClient");

		mbedtls_x509_crt_init(&_crt_auth);
		mbedtls_x509_crt_init(&_crt_user);
		mbedtls_pk_init(&_pk_crt_user);

		set_timeout(10000, 10000);
	}


	PortalClient::~PortalClient()
	{
		DEBUG_DTOR(_logger, "PortalClient");

		mbedtls_x509_crt_free(&_crt_auth);
		mbedtls_x509_crt_free(&_crt_user);
		mbedtls_pk_free(&_pk_crt_user);
	}


	bool PortalClient::init_ca_crt()
	{
		DEBUG_ENTER(_logger, "PortalClient", "init_ca_crt");
		tools::mbed_err rc = 0;

		// free previous ca certificate
		mbedtls_x509_crt_free(&_crt_auth);

		// get full filename
		tools::Path& crt_auth_file = _cert_files.crt_auth_file;
		const std::wstring filename{ crt_auth_file.to_string() };
		const std::string compacted{ tools::wstr2str(crt_auth_file.compact(40)) };

		if (tools::file_exists(filename)) {

			rc = mbedtls_x509_crt_parse_file(&_crt_auth, tools::wstr2str(filename).c_str());
			if (rc != 0) {
				_logger->info("WARNING: failed to load CA cert file %s ", compacted.c_str());
				_logger->info("%s", tools::mbed_errmsg(rc).c_str());

				goto error;
			}

			_logger->info(">> CA cert loaded from file '%s'", compacted.c_str());
		}
		else {
			_logger->error("ERROR: can't find CA cert file %s", compacted.c_str());
			mbedtls_x509_crt_init(&_crt_auth);
		}

		set_ca_crt(&_crt_auth);

	error:
		return rc == 0;
	}


	bool PortalClient::init_user_crt()
	{
		DEBUG_ENTER(_logger, "PortalClient", "init_user_crt");
		tools::mbed_err rc = 0;

		// free previous user certificate and key
		mbedtls_x509_crt_free(&_crt_user);
		mbedtls_pk_free(&_pk_crt_user);

		// get full filename
		const tools::Path& crt_user_file = _cert_files.crt_user_file;
		const std::wstring filename{ crt_user_file.to_string() };
		
		if (tools::file_exists(filename)) {
			rc = mbedtls_x509_crt_parse_file(
				&_crt_user, 
				tools::wstr2str(filename).c_str()
			);
			if (rc != 0) {
				_logger->info(
					"ERROR: failed to load user certificate from %s ", 
					tools::wstr2str(crt_user_file.filename())
				);
				_logger->info("%s", tools::mbed_errmsg(rc).c_str());

				goto error;
			}

			rc = mbedtls_pk_parse_keyfile(
				&_pk_crt_user, 
				tools::wstr2str(filename).c_str(), 
				_cert_files.crt_user_password.c_str()
			);
			if (rc != 0) {
				_logger->info(
					"ERROR: failed to load user private key from %s ", 
					tools::wstr2str(crt_user_file.filename()).c_str()
				);
				_logger->info("%s", tools::mbed_errmsg(rc).c_str());

				goto error;
			}

			set_own_crt(&_crt_user, &_pk_crt_user);
			if (rc != 0) {
				_logger->info(
					"ERROR: failed to assign user certificate from %s ", 
					tools::wstr2str(crt_user_file.filename()).c_str()
				);
				_logger->info("%s", tools::mbed_errmsg(rc).c_str());

				goto error;
			}

			_logger->info(
				">> user certificate loaded from file '%s'", 
				tools::wstr2str(crt_user_file.filename()).c_str()
			);
		}

	error:
		return rc == 0;
	}


	portal_err PortalClient::open(confirm_crt_fn confirm_crt)
	{
		DEBUG_ENTER(_logger, "PortalClient", "open");
		Mutex::Lock lock{ _mutex };
		http::Answer answer;

		_logger->info(">> connecting to %s", host().to_string().c_str());

		// initialize our certificates
		init_ca_crt();
		init_user_crt();

		// connect to the server
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

		// compute the digest of the certificate.
		_peer_crt_digest = CrtDigest(get_peer_crt());

		// get the top page
		if (!request(http::Request::GET_VERB, http::Url("/"), "", http::Headers(), answer, true))
			return portal_err::COMM_ERROR;

		// check if the web page exists
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
		
		return request(http::Request::POST_VERB, make_url("/remote/logincheck"), params.join("&"), headers, answer, false);
	}


	portal_err PortalClient::login(ask_credential_fn ask_credential, ask_code_fn ask_code)
	{
		DEBUG_ENTER(_logger, "PortalClient", "login");
		tools::Mutex::Lock lock{ _mutex };

		// misc initializations
		int retcode = 0;
		tools::StringMap params_query;
		tools::StringMap params_result;
		http::Answer answer;
		std::string body;
		Credential credential;

		// get the login page from the firewall web portal
		const http::Url login_url = make_url("/remote/login", "lang=en");
		if (!request(http::Request::GET_VERB, login_url, "", http::Headers(), answer, true))
			goto comm_error;

		// check if page is available
		if (answer.get_status_code() != HttpsClient::STATUS_OK)
			goto http_error;

		// check if the client is authenticated.
		if (authenticated()) {
			return portal_err::NONE;
		}

		_cookies.remove("SVPNCOOKIE");

		// ask for credential to user
		if (!ask_credential(credential))
			goto terminate;

		// prepare the login form
		params_query.set("ajax", "1");
		params_query.set("username", HttpsClient::url_encode(credential.username));
		params_query.set("credential", HttpsClient::url_encode(credential.password));

		// safe erase password stored in memory
		tools::serase(credential.password);

		// send the login form
		if (!login_check(params_query, answer))
			goto comm_error;

		if (!(answer.get_status_code() == HttpsClient::STATUS_OK ||
			answer.get_status_code() == HttpsClient::STATUS_UNAUTHORIZED ||
			answer.get_status_code() == HttpsClient::STATUS_FORBIDDEN))
			goto http_error;

		// get the body from the answer
		body = std::string(answer.body().cbegin(), answer.body().cend());

		// check if the return code is present
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
					if (!extract_redir_url(params_result, redir_url))
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
					if (!extract_redir_url(params_result, redir_url))
						goto redir_error;

					if (!request(http::Request::GET_VERB, redir_url, "", http::Headers(), answer, false))
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
					Code2FA code2fa;
					std::string device;
					if (params_result.get_str("tokeninfo", device)) {
						device = http::HttpsClient::url_decode(device);
						code2fa.info = "Authentication code for " + device;
					}
					else {
						code2fa.info = "Authentication code";
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
						Code2FA challenge;
						
						// .. assign a default message
						challenge.info = "enter code";
						params_result.get_str("chal_msg", challenge.info);

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
		_logger->error("ERROR: portal login failure - %s (HTTP code %d)",
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


	void PortalClient::logoff()
	{
		DEBUG_ENTER(_logger, "PortalClient", "logoff");
		tools::Mutex::Lock lock{ _mutex };

		// Delete all session cookies
		_cookies.clear();
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
		bool ok = request(http::Request::GET_VERB, portal_url, "", headers, answer, false);
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
		if (!request(http::Request::GET_VERB, vpninfo_url, "", headers, answer, false))
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
		http::Request request{ http::Request::GET_VERB, tunnel_url, _cookies };
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
		return _cookies.exists("SVPNCOOKIE") && 
			_cookies.get("SVPNCOOKIE").get_value().length() > 0;
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
		http::Request request{ verb, url, _cookies };

		request.headers()
			.set("Accept", "text/html")
			.set("Accept-Encoding", "gzip")
			.set("Accept-Encoding", "identity")
			.set("Accept-Language", "en")
			.set("Cache-Control", "no-cache")
			.set("Connection", "keep-alive")
			.set("Host", host().to_string())
			.set("User-Agent", "Mozilla/5.0")
			.add(headers);

		request.set_body((unsigned char *)body.c_str(), body.length());

		// Send and wait for a response
		const bool success = PortalClient::send_and_receive(request, answer);
		if (success) {
			_cookies.add(answer.cookies());
		}
		else {
			// disconnect if not yet done
			disconnect();
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
		const std::string& body, const http::Headers& headers, http::Answer& answer, bool allow_redir)
	{
		if (!do_request(verb, url, body, headers, answer))
			return false;

		// redirect if required
		if (answer.get_status_code() == HttpsClient::STATUS_FOUND && allow_redir) {
			std::string location;
			answer.headers().get("location", location);

			const http::Url redir_url{ location };
			if (!do_request(verb, redir_url, body, http::Headers(), answer))
				return false;
		}

		return true;
	}


	bool PortalClient::extract_redir_url(const tools::StringMap& params, http::Url& url)
	{
		std::string redir;
		if (!params.get_str("redir", redir))
			return false;
		
		url = http::Url{ url_decode(redir) };
		return true;
	}

}
