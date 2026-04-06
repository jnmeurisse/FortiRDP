/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include <functional>
#include "http/Url.h"
#include "http/cookies.h"
#include "net/Endpoint.h"
#include "util/Credential.h"


namespace fw {

	// SSL VPN supported authentication methods.
	enum class AuthMethod {
		DEFAULT,        // Default mode configured in the user interface.
		BASIC,          // SSL VPN with username and password (includes MFA)
		CERTIFICATE,    // SSL VPN with certification authentication
		SAML            // SSL VPN with SAML IdP
	};


	class AuthRequest
	{
	protected:
		explicit AuthRequest(const std::wstring prompt, const net::Endpoint& endpoint) :
			prompt(prompt),
			endpoint(endpoint)
		{
		}

	public:
		// Prompt to show when requesting an authentication code.
		const std::wstring prompt;

		// Endpoint for which credentials are requested.
		const net::Endpoint& endpoint;
	};


	// SSL VPN User's Credential Request
	class AuthCredentialRequest : public AuthRequest
	{
	public:
		explicit AuthCredentialRequest(const std::wstring prompt, const net::Endpoint& endpoint) :
			AuthRequest(prompt, endpoint)
		{}

		void clear() {
			credentials.clear();
		}

		// Credentials provided by the user
		utl::Credential credentials;
	};

	// SSL VPN MFA authentication code request
	class AuthCodeRequest : public AuthRequest
	{
	public:
		explicit AuthCodeRequest(const std::wstring prompt, const net::Endpoint& endpoint) :
			AuthRequest(prompt, endpoint)
		{}

		std::wstring code;
	};

	// SSL VPN SAML authentication configuration.
	struct AuthSamlInfo {
		// FortiGate Service provider URL.
		http::Url service_provider_url;

		// FortiGate certificate.  The certificate was validated
		// during the initial connection.
		std::string service_provider_crt;

		// A reference to the application cookie jar.
		http::Cookies& cookie_jar;

		// A function that returns true if the SAML service provider is
		// authenticated.  The function checks if the session cookie has a
		// value.
		std::function<bool()> is_authenticated;
	};

}
