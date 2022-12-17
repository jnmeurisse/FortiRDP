/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <functional>

#include "http/HttpsClient.h"
#include "http/Answer.h"
#include "http/Cookies.h"
#include "http/Url.h"

#include "fw/CrtThumbprint.h"

#include "net/Endpoint.h"
#include "net/Tunneler.h"

#include "tools/Mutex.h"
#include "tools/Path.h"
#include "tools/ErrUtil.h"

namespace fw {

	// credential definition
	struct Credential
	{
		std::wstring username;
		std::wstring password;
	};

	// code for a 2 factor authentication
	struct Code2FA 
	{
		std::string info;
		std::string code;
	};

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
	typedef enum {
		NONE = 0,
		COMM_ERROR,
		CERT_UNTRUSTED,
		HTTP_ERROR,
		ACCESS_DENIED,
		LOGIN_CANCELLED
	} portal_err;


	// Callbacks definition
	using confirm_crt_fn = std::function<bool (const mbedtls_x509_crt*, int)>;
	using ask_credential_fn = std::function<bool (Credential&)>;
	using ask_code_fn = std::function<bool (Code2FA&)>;

	/**
	 * A fortigate sslvpn portal client
	 */
	class PortalClient final : public http::HttpsClient
	{
	public:
		explicit PortalClient(const net::Endpoint& ep);
		~PortalClient();

		/* Sets the root CA certificate file used to authenticate the server
		*/
		bool set_ca_file(const tools::Path& ca_file);

		/* Returns the name of the root CA file
		*/
		inline const tools::Path& get_ca_file() const noexcept { return _ca_file; }

		/* Set the user certificate file
		*/
		bool set_crt_file(const tools::Path& crt_file);

		/* Set the private key file
		*/
		bool set_pk_file(const tools::Path& pk_file);

		/* Opens the connection. The confirm_crt_fn function is called when
		 * the user is asked to accept the server certificate. The method returns
		 * ERR_NONE if the connection opens
		*/
		portal_err open(confirm_crt_fn confirm_crt);

		/* Logins into the portal. The ask_code function is called when
		 * the user must enter an additional authentication code. 
		 * The method returns ERR_NONE if authentication succeeds.
		*/
		portal_err login(ask_credential_fn ask_credential, ask_code_fn ask_code);

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

		/* Returns true if we are authenticated in the portal
		*/
		inline bool authenticated() const {return _authenticated;}

	private:
		// The CA public certificate filename
		tools::Path _ca_file;

		// The CA public certificate if loaded.
		mbedtls_x509_crt _ca_crt;

		// Our own certificate filename and private key
		tools::Path _crt_file;
		tools::Path _pk_file;
		
		// Our own certificate and private key if loaded
		mbedtls_x509_crt _own_crt;
		mbedtls_pk_context _own_key;

		// The peer certificate thumbprint
		CrtThumbprint _peer_crt_thumbprint;

		// Session cookies
		http::Cookies _cookies;

		// true if socket connected
		bool _connected;

		// true if authenticated
		bool _authenticated;

		// mutex to serialize calls
		tools::Mutex _mutex;

		// Sends a request and wait for a response.
		bool send_and_receive(http::Request& request, http::Answer& answer);

		// Sends a request and wait for a response.
		bool do_request(const std::string& verb, const http::Url& url, const std::string& body,
			const http::Headers& headers, http::Answer& answer);

		// Sends a request and wait for a response, follows redirect if allow_redir is true
		bool request(const std::string& verb, const http::Url& url, const std::string& body,
			const http::Headers& headers, http::Answer& answer, bool allow_redir);

		// Sends a login check to the firewall portal
		bool login_check(const tools::StringMap& params, http::Answer& answer);
	};

}