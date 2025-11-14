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

namespace fw {

	// SSL VPN supported authentication methods.
	enum class AuthMethod {
		DEFAULT,        // Default mode configured in the user interface.
		BASIC,          // SSL VPN with username and password (includes MFA)
		CERTIFICATE,    // SSL VPN with certification authentication
		SAML            // SSL VPN with SAML IdP
	};

	// SSL VPN User Credentials
	struct AuthCredentials
	{
		std::wstring username;
		std::wstring password;
	};

	// SSL VPN MFA authentication code.
	struct AuthCode
	{
		std::string prompt;
		std::string code;
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
