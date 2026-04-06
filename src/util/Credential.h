/*!
* This file is part of FortiRDP
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once
#include <string>

namespace utl {
	struct Credential {
		Credential() = default;
		~Credential();

		/**
		 * Clears the username and password.  The password is cleared using
		 * SecureZeroMemory function.
		 */
		void clear();

		std::wstring username;
		std::wstring password;
	};
}
