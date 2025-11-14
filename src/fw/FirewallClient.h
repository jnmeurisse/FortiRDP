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
#include "fw/FirewallTunnel.h"
#include "net/Endpoint.h"
#include "tools/Mutex.h"
#include "tools/StringMap.h"



namespace fw {

	// Portal info
	struct PortalInfo
	{
		std::string user;			// username connected to the portal
		std::string group;			// group name this user belong to
		std::string version;		// portal version
	};

	// SSLVPN configuration.
	struct SslvpnConfig
	{
		std::string local_addr;		// IP address assigned to this client.
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


	// Callback definitions
	using confirm_crt_fn = std::function<bool (const mbedtls_x509_crt*, int)>;
	using ask_credentials_fn = std::function<bool (AuthCredentials&)>;
	using ask_pincode_fn = std::function<bool (AuthCode&)>;
	using ask_samlauth_fn = std::function<bool (AuthSamlInfo&)>;


	/**
	 * A FortiGate SSLVPN portal client
	 */
	class FirewallClient final : public http::HttpsClient
	{
	public:
		explicit FirewallClient(const net::Endpoint& ep, const std::string& realm, const net::TlsConfig& config);
		~FirewallClient();

		/**
		 * Opens a connection to the FortiGate portal and performs certificate validation.
		 *
		 * This function attempts to connect to the web server, handles certificate validation,
		 * and sends a request to retrieve the top page. The function logs relevant
		 * information during each step, including the connection details and certificate
		 * status. If the certificate is untrusted, it invokes the `confirm_crt` callback
		 * to validate whether the connection should proceed.
		 *
		 * @param confirm_crt A callback function that is called to confirm the certificate
		 *                    status if it is untrusted. The callback should return `true`
		 *                    to proceed with the connection or `false` to abort.
		 *
		 * @return A `portal_err` value indicating the result of the connection attempt:
		 *         - `portal_err::NONE` if the connection was successful.
		 *         - `portal_err::COMM_ERROR` if the connection or communication failed.
		 *         - `portal_err::CERT_UNTRUSTED` if the certificate could not be trusted.
		 *         - `portal_err::HTTP_ERROR` if the portal page retrieval failed.
		 */
		portal_err open(confirm_crt_fn confirm_crt);

		/**
		 * Performs basic authentication with the FortiGate portal using the provided credentials.
		 *
		 * This function sends a login request to the server, retrieves the status, and
		 * handles various authentication flows, including two-factor authentication (2FA),
		 * password renewal, and challenges. It uses two callback functions to ask the
		 * user for credentials and a 2FA code when needed.
		 *
		 * @param ask_credentials_fn ask_credential A callback function used to prompt
		 *                           the user for the username and password.
		 * @param ask_pincode_fn ask_code A callback function used to prompt the user
		 *                               for the 2FA code or challenge response.
		 *
		 * @return portal_err::NONE if login is successful, or an error code based on
		 *         different failure scenarios:
		 *         - portal_err::COMM_ERROR for communication errors.
		 *         - portal_err::HTTP_ERROR for HTTP status code errors.
		 *         - portal_err::ACCESS_DENIED for denied access.
		 *         - portal_err::CERT_UNTRUSTED if certificate verification fails.
		 *         - portal_err::LOGIN_CANCELLED if the login is canceled by the user.
		 */
		portal_err login_basic(ask_credentials_fn ask_credential, ask_pincode_fn ask_code);

		/**
		 * Performs SAML-based authentication with the portal server.
		 *
		 * This function initiates the SAML authentication process by gathering necessary
		 * information, including the service provider certificate, and then uses the
		 * provided callback to request the user to authenticate via SAML. If the user
		 * completes the authentication successfully, the function proceeds; otherwise,
		 * it returns a login canceled error.
		 *
		 * @param ask_samlauth_fn ask_samlauth A callback function that prompts the
		 *                                user for the SAML authentication process.
		 *
		 * @return - portal_err::NONE if login is successful, or an error code:
		 *         - portal_err::CERT_INVALID if the certificate is invalid.
		 *         - portal_err::LOGIN_CANCELLED if the login is canceled by the user.
		 */
		portal_err login_saml(ask_samlauth_fn ask_samlauth);

		/* Log out from the portal.
		*/
		bool logout();

		/**
		 * Retrieves information about the portal.
		 *
		 * This function makes an authenticated request to the portal server to fetch
		 * details about the portal, including user, group, and version information. If
		 * the client is not authenticated, the function returns false. If the request is
		 * successful, the portal information is parsed and stored in the provided
		 * `PortalInfo` object.
		 *
		 * @param portal_info The object that will receive the portal information.
		 *                    The object will be updated with the user, group, and
		 *                    version information from the portal.
		 *
		 * @return true if the information is successfully retrieved and parsed;
		 *         false if the client is not authenticated or if the request fails.
		 */
		bool get_info(PortalInfo& portal_info);

		/**
		 * Retrieves the SSL VPN configuration.
		 *
		 * This function makes an authenticated request to the portal server to fetch the
		 * SSL VPN configuration in XML format. It is mandatory to obtain this configuration
		 * to get the IP address from the FortiGate device. If the request is successful,
		 * the local address (IPv4) is extracted and stored in the provided `SslvpnConfig`
		 * object. If the client is not authenticated or if there are issues with the
		 * request or XML parsing, the function returns `false`.
		 *
		 * @param sslvpn_config The object to store the SSL VPN configuration, particularly
		 *                      the assigned local IPv4 address.
		 *
		 * @return true if the configuration is successfully retrieved and parsed;
		 *         false if the client is not authenticated, the request fails, or if
		 *         there are issues parsing or decoding the XML response.
		 */
		bool get_config(SslvpnConfig& sslvpn_config);

		/**
		 * Allocates a firewall tunnel between a local and a remote endpoint.
		 *
		 * This function creates a tunnel instance configured to forward traffic
		 * between the specified local and remote endpoints. However, the tunnel
		 * is not established upon creation. The caller must explicitly invoke
		 * `connect()` on the returned tunnel object to establish the connection.
		 *
		 * @param local_ep The local network endpoint for the tunnel.
		 * @param remote_ep The remote network endpoint to which traffic is forwarded.
		 * @param config The configuration parameters for the tunneler.
		 * @return A pointer to the allocated FirewallTunnel instance, or nullptr if
		 *         the tunnel could not be created.
		 */
		fw::FirewallTunnel* create_tunnel(const net::Endpoint& local_ep, const net::Endpoint& remote_ep,
			const net::tunneler_config& config);

		/**
		 * Returns true if this client is authenticated on the portal.
		*/
		bool is_authenticated() const;


	private:
		// The peer certificate digest.
		CrtDigest _peer_crt_digest;

		// Session cookies.
		http::Cookies _cookie_jar;

		// Mutex to serialize calls.
		tools::Mutex _mutex;

		// The fortiGate realm.
		const std::string _realm;

		// Logs an HTTP error message.
		void log_http_error(const char* msg, const http::Answer& answer);

		// Sends a request and wait for a response.
		bool send_and_receive(http::Request& request, http::Answer& answer);

		// Sends a request and wait for a response.
		bool do_request(const std::string& verb, const http::Url& url, const std::string& body,
			const http::Headers& headers, http::Answer& answer);

		// Sends a request and wait for a response, follows redirect if allow_redir is >= 0.
		// allow_redir specifies the number of allowed redirection.
		bool request(const std::string& verb, const http::Url& url, const std::string& body,
			const http::Headers& headers, http::Answer& answer, int allow_redir);

		// Sends a login check to the firewall portal.
		portal_err try_login_check(const tools::StringMap& in_params, tools::StringMap& out_params);

		// Gets the redirect URL from the given map.
		bool get_redir_url(const tools::StringMap& params, http::Url& url) const;
	};

}
