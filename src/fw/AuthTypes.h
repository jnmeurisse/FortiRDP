/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include "http/Url.h"
#include "http/cookie.h"

namespace fw {

	enum class AuthMethod {
		DEFAULT,
		BASIC,
		CERTIFICATE,
		SAML
	};

	// User Credentials
	struct UserCredentials
	{
		std::wstring username;
		std::wstring password;
	};

	// Code
	struct PinCode
	{
		std::string prompt;
		std::string code;
	};


	struct SamlAuthInfo {
		http::Url service_provider_url;
		std::string service_provider_crt;
		std::string service_provider_auth_cookie;
	};

}
