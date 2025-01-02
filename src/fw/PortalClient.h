/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <functional>
#include <string>
#include <mbedtls/x509_crt.h>
#include "fw/AuthTypes.h"
#include "http/HttpsClient.h"
#include "http/Answer.h"
#include "http/Cookies.h"
#include "http/Url.h"
#include "http/Request.h"
#include "http/Headers.h"
#include "fw/CrtDigest.h"
#include "net/Endpoint.h"
#include "net/Tunneler.h"
#include "tools/Mutex.h"
#include "tools/StringMap.h"



namespace fw {

	// portal info
	struct PortalInfo
	{
		std::string user;
		std::string group;
		std::string version;
	};

	// sslvpn config
	struct SslvpnConfig
	{
		std::string local_addr;
	};


	// Portal Client error codes
	enum class portal_err {
		NONE = 0,
		COMM_ERROR,
		CERT_UNTRUSTED,
		CERT_INVALID,
		HTTP_ERROR,
		ACCESS_DENIED,
		LOGIN_CANCELLED
	};


	// Callbacks definition
	using confirm_crt_fn = std::function<bool (const mbedtls_x509_crt*, int)>;
	using ask_credentials_fn = std::function<bool (AuthCredentials&)>;
	using ask_pincode_fn = std::function<bool (AuthCode&)>;
	using ask_samlauth_fn = std::function<bool (AuthSamlInfo&)>;

	/**
	 * A fortigate sslvpn portal client
	 */
	class PortalClient final : public http::HttpsClient
	{
	public:
		explicit PortalClient(const net::Endpoint& ep);
		~PortalClient();

		/* Opens the connection. The confirm_crt_fn function is called when
		 * the user is asked to accept the server certificate. The method returns
		 * ERR_NONE if the connection opens
		*/
		portal_err open(confirm_crt_fn confirm_crt);

		/* Logins into the portal. The ask_code function is called when
		 * the user must enter an additional authentication code. 
		 * The method returns ERR_NONE if authentication succeeds.
		*/
		portal_err login_basic(ask_credentials_fn ask_credential, ask_pincode_fn ask_code);

		portal_err login_saml(ask_samlauth_fn ask_samlauth);

		/* Log-offs from the portal.
		*/
		void logoff();

		/* Starts the tunnel mode 
		*/
		bool start_tunnel_mode();

		/* Get portal info : username, group membership
		*/
		bool get_info(PortalInfo& portal_info);

		/* Get vpn configuration
		*/
		bool get_config(SslvpnConfig& sslvpn_config);

		/* Creates the tunneler from the local end point to the remote end point.
		 * The remote end point is located behind the firewall.
		*/
		net::Tunneler* create_tunnel(const net::Endpoint& local, const net::Endpoint& remote, const net::tunneler_config& config);

		/* Returns true if we are authenticated on the portal
		*/
		bool authenticated() const;

	private:
		// The peer certificate digest
		CrtDigest _peer_crt_digest;

		// Session cookies
		http::Cookies _cookie_jar;

		// mutex to serialize calls
		tools::Mutex _mutex;

		// Sends a request and wait for a response.
		bool send_and_receive(http::Request& request, http::Answer& answer);

		// Sends a request and wait for a response.
		bool do_request(const std::string& verb, const http::Url& url, const std::string& body,
			const http::Headers& headers, http::Answer& answer);

		// Sends a request and wait for a response, follows redirect if allow_redir is true.
		bool request(const std::string& verb, const http::Url& url, const std::string& body,
			const http::Headers& headers, http::Answer& answer, bool allow_redir);

		// Sends a login check to the firewall portal.
		bool login_check(const tools::StringMap& params, http::Answer& answer);

		// Gets the redirect URL from the given map.
		bool get_redir_url(const tools::StringMap& params, http::Url& url) const;
	};

}
