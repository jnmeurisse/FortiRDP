/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once
#include <Windows.h>
#include <string>
#include "mbedTLS/ssl.h"
#include "fw/PortalClient.h"
#include "ui/SyncProc.h"


/**
* A synchronous procedure that connects this client to the firewall. The procedure
* posts a ConnectedEvent to the window when done.
*/
class SyncConnect final : public SyncProc
{
public:
	explicit SyncConnect(HWND hwnd, fw::PortalClient* portal);
	~SyncConnect();

private:
	fw::PortalClient* const _portal;

	// send a show error message command to the window specified by hwnd
	void showErrorMessageDialog(const wchar_t* message) const;

	// callback called to ask user to confirm certificate usage
	bool confirm_certificate(const mbedtls_x509_crt* crt, int status);

	// callback called to ask user to provide user name/password
	bool ask_credential(fw::Credential& credential);

	// callback called to ask user to provide 2fa code
	bool ask_code(fw::Code2FA& code2fa);

	// connect procedure
	virtual bool procedure() override;
};

