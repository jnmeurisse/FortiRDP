/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "mbedtls/x509.h"

#include "PortalClient.h"
#include "http/Request.h"
#include "http/Cookie.h"
#include "http/Cookies.h"
#include "http/Url.h"

#include "tools/StringMap.h"
#include "tools/SysUtil.h"
#include "tools/ErrUtil.h"
#include "tools/Json11.h"
#include "tools/pugixml.hpp"

namespace fw {

	PortalClient::PortalClient(const net::Endpoint& ep) :
		HttpsClient(ep),
		_connected(false),
		_authenticated(false),
		_peer_crt_thumbprint(),
		_mutex(),
		_ca_file(),
		_cookies()
	{
		DEBUG_CTOR(_logger, "PortalClient");

		mbedtls_x509_crt_init(&_ca_crt);
		set_timeout(10000, 10000);
	}


	PortalClient::~PortalClient()
	{
		DEBUG_DTOR(_logger, "PortalClient");

		mbedtls_x509_crt_free(&_ca_crt);
	}


	bool PortalClient::set_ca_file(const tools::Path& ca_file)
	{
		DEBUG_ENTER(_logger, "PortalClient", "set_ca_file");
		tools::mbed_err rc = 0;

		// free previous ca certificate
		mbedtls_x509_crt_free(&_ca_crt);

		// save the file
		_ca_file = ca_file;

		// get full filename
		const std::wstring filename(ca_file.to_string());

		if (tools::file_exists(filename)) {
			const std::string compacted(tools::wstr2str(ca_file.compact(50)));

			rc = mbedtls_x509_crt_parse_file(&_ca_crt, tools::wstr2str(filename).c_str());
			if (rc != 0) {
				_logger->info("WARNING: failed to load CA certificate from file %s ", compacted.c_str());
				_logger->info("%s", tools::mbed_errmsg(rc).c_str());

				goto error;
			}

			_logger->info(">> certificate loaded from file '%s'", compacted.c_str());
		}
		else {
			mbedtls_x509_crt_init(&_ca_crt);
		}

		set_ca_crt(&_ca_crt);

	error:
		return rc == 0;
	}


	portal_err PortalClient::open(confirm_crt_fn confirm_crt)
	{
		DEBUG_ENTER(_logger, "PortalClient", "open");
		tools::Mutex::Lock lock(_mutex);
		tools::mbed_err rc = 0;
		http::Answer answer;

		_logger->info(">> connecting to %s", host().to_string().c_str());

		// define the type of cipher used to encrypt and sign all traffic 
		set_cipher(Cipher::HIGH_SEC);

		// connect to the server
		if ((rc = HttpsClient::connect()) != 0) {
			_logger->error("ERROR: failed to connect to %s", host().to_string().c_str());
			_logger->error("%s", tools::mbed_errmsg(rc).c_str());

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

		// compute the thumbprint of the certificate.
		_peer_crt_thumbprint = CrtThumbprint(*get_peer_crt());

		// get the top page
		if (!request(http::Request::GET_VERB, "/", "", http::Headers(), answer))
			return portal_err::COMM_ERROR;

		// check if the web page exists
		if (answer.get_status_code() != HttpsClient::STATUS_OK) {
			_logger->error("ERROR: portal connection failure - %s (HTTP code %d)", 
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
		headers.set("Pragma", "no-cache");

		return request(http::Request::POST_VERB, "/remote/logincheck", params.join("&"), headers, answer);
	}


	portal_err PortalClient::login(ask_credential_fn ask_credential, ask_code_fn ask_code)
	{
		DEBUG_ENTER(_logger, "PortalClient", "login");
		tools::Mutex::Lock lock(_mutex);

		// misc initializations
		int retcode = 0;
		tools::StringMap params_query;
		tools::StringMap params_result;
		http::Answer answer;
		std::string body;
		Credential credential;

		// the SSLVPN cookie must be removed to be unauthenticated
		_authenticated = false;
		_cookies.clear();

		// get the login page from the firewall web portal
		if (!request(http::Request::GET_VERB, "/remote/login?lang=en", "", http::Headers(), answer))
			goto comm_error;

		// redirect to the login page if firewall language is different
		if (answer.get_status_code() == HttpsClient::STATUS_FOUND) {
			std::string location;
			answer.headers().get("location", location);
			if (!request(http::Request::GET_VERB, location, "", http::Headers(), answer))
				goto comm_error;
		}

		// check if page is available
		if (answer.get_status_code() != HttpsClient::STATUS_OK)
			goto http_error;

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
			while (!_authenticated) {
				std::string redir_url;
				params_result.get_str("redir", redir_url);

				switch (retcode) {
				case 0: {
					// Access denied,
					// show the error message received from the firewall
					const http::Url url(url_decode(redir_url));
					std::string msg;
					if (url.get_query().get_str("err", msg)) {
						_logger->error("ERROR: %s", msg.c_str());
					}
				}
				goto auth_error;
				break;

				case 1: {
					// Access allowed
					if (!request(http::Request::GET_VERB, redir_url, "", http::Headers(), answer))
						goto comm_error;

					if (answer.get_status_code() != HttpsClient::STATUS_OK)
						goto http_error;

					_authenticated = true;
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
					_logger->error("ERROR: unknown return code %d during authentication.", retcode);
					goto terminate;
				}
			}
		}
		else {
			// Firewall should be running an old FortiOS 3
			// check is svpncookie is assigned
			if (_cookies.exists("SVPNCOOKIE")) {
				const http::Cookie cookie = _cookies.get("SVPNCOOKIE");
				_authenticated = cookie.get_value().size() > 0;
				if (!_authenticated)
					goto auth_error;

			}
			else {
				_logger->error("ERROR: invalid firewall answer");
				goto terminate;
			}
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

	terminate:
		return portal_err::LOGIN_CANCELLED;
	}


	void PortalClient::logoff()
	{
		DEBUG_ENTER(_logger, "PortalClient", "logoff");
		tools::Mutex::Lock lock(_mutex);

		if (_authenticated) {
			_authenticated = false;
			_cookies.clear();
		}
	}


	bool PortalClient::get_info(PortalInfo& portal_info)
	{
		DEBUG_ENTER(_logger, "PortalClient", "get_info");
		tools::Mutex::Lock lock(_mutex);
		bool ok = false;

		if (_authenticated) {
			http::Headers headers;
			http::Answer answer;

			ok = request(http::Request::GET_VERB, "/remote/portal?access", "", headers, answer);
			if (ok && answer.get_status_code() == HttpsClient::STATUS_OK) {
				const tools::ByteBuffer& body = answer.body();
				const std::string data(body.cbegin(), body.cend());
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

					ok = true;
				}
			}
		}

		return ok;
	}


	bool PortalClient::get_config(SslvpnConfig& sslvpn_config)
	{
		DEBUG_ENTER(_logger, "PortalClient", "get_config");
		tools::Mutex::Lock lock(_mutex);
		bool ok = false;

		if (_authenticated) {
			http::Headers headers;
			http::Answer answer;
			ok = request(http::Request::GET_VERB, "/remote/fortisslvpn_xml", "", headers, answer);

			if (ok && answer.get_status_code() == HttpsClient::STATUS_OK) {
				const tools::ByteBuffer& body = answer.body();
				const std::string data(body.cbegin(), body.cend());

				pugi::xml_document doc;
				pugi::xml_parse_result parse_result = doc.load_string(data.c_str());

				if (parse_result.status != pugi::xml_parse_status::status_ok) {
					//TODO: report an error
				}
				else {
					const pugi::xml_attribute address = doc
														.child("sslvpn-tunnel")
														.child("ipv4")
														.child("assigned-addr")
														.attribute("ipv4");
					sslvpn_config.local_addr = address.as_string();
				}
			}
		}

		return ok;
	}

	bool PortalClient::start_tunnel_mode()
	{
		DEBUG_ENTER(_logger, "PortalClient", "start_tunnel_mode");
		bool started = false;
		tools::Mutex::Lock lock(_mutex);

		http::Answer answer;
		http::Request request(http::Request::GET_VERB, "/remote/sslvpn-tunnel", _cookies, 1);
		request.headers().set("Host", "sslvpn");

		try {
			send_request(request);
			started = true;
		}
		catch (const tools::tnl_error& e) {
			_logger->error("ERROR: failed to open the tunnel");
			_logger->error("%s", e.message());
		}

		return started;
	}


	net::Tunneler* PortalClient::create_tunnel(
		const net::Endpoint& local, const net::Endpoint& remote, const net::tunneler_config& config)
	{
		DEBUG_ENTER(_logger, "PortalClient", "create_tunnel");

		return new net::Tunneler(*this, local, remote, config);
	}


	bool PortalClient::request(const std::string& verb, const std::string& url,
		const std::string& body, const http::Headers& headers, http::Answer& answer)
	{
		DEBUG_ENTER(_logger, "PortalClient", "request");
		
		// Prepare the request
		http::Request request(verb, url, _cookies);

		request.headers()
			.set("Host", host().to_string())
			.set("Connection", "keep-alive")
			.set("Accept", "text/html")
			.set("User-Agent", "Mozilla/5.0")
			.add(headers);

		request.set_body((unsigned char *)body.c_str(), body.length());

		bool ok;
		int retry = 0;
		do {
			ok = PortalClient::request(request, answer);
			if (ok) {
				_cookies.add(answer.cookies());
			}
			else {
				disconnect();
			}

			retry++;
		} while (!ok && retry <= 1);

		_logger->debug("... %s https://%s%s (status=%s (%d))",
			verb.c_str(),
			host().to_string().c_str(),
			url.c_str(),
			answer.get_reason_phrase().c_str(),
			answer.get_status_code());

		return ok;
	}


	bool PortalClient::request(http::Request& request, http::Answer& answer)
	{
		DEBUG_ENTER(_logger, "PortalClient", "request");
		int rc;

		if (must_reconnect()) {
			disconnect();

			if ((rc = connect()) != 0) {
				_logger->error("ERROR: failed to connect to %s", host().to_string().c_str());
				_logger->error("%s", tools::mbed_errmsg(rc).c_str());

				return false;
			}

			// verify the thumbprint of the certificate
			if (_peer_crt_thumbprint != CrtThumbprint(*get_peer_crt())) {
				_logger->error("ERROR: invalid certificate thumbprint");

				return false;
			}
		}

		//.. send the request
		try {
			send_request(request);
		}
		catch (const tools::tnl_error& e) {
			_logger->error("ERROR: failed to send data to %s", host().to_string().c_str());
			_logger->error("%s", e.message().c_str());
			
			return false;
		}

		//.. receive the answer
		try {
			if (!recv_answer(answer)) {
				_logger->error("ERROR: failed to receive data from %s", host().to_string().c_str());
				_logger->error("Communication timeout");
			}
		}
		catch (const tools::tnl_error& e) {
			_logger->error("ERROR: failed to receive data from %s", host().to_string().c_str());
			_logger->error("%s", e.message().c_str());

			return false;
		}

		return true;
	}
}