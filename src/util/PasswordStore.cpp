/*!
* This file is part of FortiRDP
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "PasswordStore.h"

#include <windows.h>
#include <wincred.h>

namespace utl {
	static wchar_t* target = L"fortirdp";


	PasswordStore& PasswordStore::instance()
	{
		static PasswordStore instance;
		return instance;
	}


	bool PasswordStore::save(const std::wstring& username, const std::wstring& secret)
	{
		CREDENTIAL cred{};

		cred.Flags = 0;
		cred.Type = CRED_TYPE_GENERIC;
		cred.TargetName = target;
		cred.CredentialBlobSize = static_cast<DWORD>(secret.size() * sizeof(wchar_t));
		cred.CredentialBlob = (LPBYTE)secret.data();
		cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
		cred.UserName = const_cast<LPWSTR>(username.c_str());

		return ::CredWrite(&cred, 0);
	}
	

	bool PasswordStore::load(const std::wstring& username, std::wstring& secret)
	{
		PCREDENTIALW pcred = nullptr;

		if (!::CredRead(target, CRED_TYPE_GENERIC, 0, &pcred))
			return false;

		secret = std::wstring(
			reinterpret_cast<wchar_t*>(pcred->CredentialBlob),
			pcred->CredentialBlobSize / sizeof(wchar_t)
		);

		CredFree(pcred);

		return true;
	}
	

	void PasswordStore::remove()
	{
		CredDelete(target, CRED_TYPE_GENERIC, 0);
	}
}