/*!
* This file is part of FortiRDP
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "Credential.h"
#include <string>


namespace utl {

	class CredentialStore {
	public:
		static CredentialStore& instance();

		bool save(const std::wstring& host, const Credential& credential);
		bool load(const std::wstring& host, Credential& credential);
		void remove(const std::wstring& host);

	private:
		CredentialStore() = default;
		~CredentialStore() = default;

		CredentialStore(const CredentialStore&) = delete;
		CredentialStore& operator=(const CredentialStore&) = delete;
	};
}
