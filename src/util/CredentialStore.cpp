/*!
* This file is part of FortiRDP
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "CredentialStore.h"

#include <windows.h>
#include <wincred.h>

namespace utl {

	static std::wstring make_target(const std::wstring& host)
	{
		static std::wstring target_prefix = L"fortirdp:";
		return target_prefix + host;
	}


	CredentialStore& CredentialStore::instance() noexcept
	{
		static CredentialStore instance;
		return instance;
	}


	bool CredentialStore::save(const std::wstring& host, const Credential& credential)
	{
		CREDENTIAL cred{};
		std::wstring target{ make_target(host) };

		cred.Flags = 0;
		cred.Type = CRED_TYPE_GENERIC;
		cred.TargetName = const_cast<LPWSTR>(target.c_str());
		cred.CredentialBlobSize = static_cast<DWORD>(credential.password.size() * sizeof(wchar_t));
		cred.CredentialBlob = (LPBYTE)credential.password.data();
		cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
		cred.UserName = const_cast<LPWSTR>(credential.username.c_str());

		return ::CredWrite(&cred, 0);
	}
	

	bool CredentialStore::load(const std::wstring& host, Credential& credential)
	{
		std::wstring target{ make_target(host) };
		PCREDENTIALW pcred = nullptr;


		if (!::CredRead(target.c_str(), CRED_TYPE_GENERIC, 0, &pcred))
			return false;

		if (pcred->CredentialBlob)
			credential.password.assign(
				reinterpret_cast<wchar_t*>(pcred->CredentialBlob),
				pcred->CredentialBlobSize / sizeof(wchar_t)
			);

		if (pcred->UserName)
			credential.username = pcred->UserName;

		CredFree(pcred);

		return true;
	}
	

	void CredentialStore::remove(const std::wstring& host)
	{
		std::wstring target{ make_target(host) };
		CredDelete(target.c_str(), CRED_TYPE_GENERIC, 0);
	}
}