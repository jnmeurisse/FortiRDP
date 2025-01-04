/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <Windows.h>
#include <mbedtls/x509_crt.h>
#include "fw/AuthTypes.h"
#include "fw/PortalClient.h"
#include "ui/SyncProc.h"


namespace ui {

	/**
	* A synchronous procedure that connects this client to the firewall. The procedure
	* posts a ConnectedEvent to the window when done.
	*/
	class SyncConnect final : public SyncProc
	{
	public:
		explicit SyncConnect(HWND hwnd, fw::AuthMethod auth_method, fw::PortalClient* portal);
		~SyncConnect();

	private:
		const fw::AuthMethod _auth_method;
		fw::PortalClient* const _portal;

		// send a show error message command to the window specified by hwnd
		void showErrorMessageDialog(const wchar_t* message) const;

		// callback called to ask user to confirm certificate usage
		bool confirm_certificate(const mbedtls_x509_crt* crt, int status);

		// callback called to ask user to provide a user name/password
		bool ask_credentials(fw::AuthCredentials& credential);

		// callback called to ask user to provide a pin code
		bool ask_pincode(fw::AuthCode& code2fa);

		// callback called when authenticating a user with SAML.
		bool ask_saml_auth(fw::AuthSamlInfo& saml_info);

		// connect procedure
		virtual bool procedure() override;
	};

}
