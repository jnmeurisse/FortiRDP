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
	class PasswordStore {
	public:
		static PasswordStore& instance();

		bool save(const std::wstring& username, const std::wstring& password);
		bool load(const std::wstring& username, std::wstring& password);
		void remove();

	private:
		PasswordStore() = default;
		~PasswordStore() = default;

		PasswordStore(const PasswordStore&) = delete;
		PasswordStore& operator=(const PasswordStore&) = delete;
	};
}
