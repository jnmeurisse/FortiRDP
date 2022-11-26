/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "tools/Path.h"
#include "tools/SysUtil.h"

#include "ui/SyncConnect.h"


SyncConnect::SyncConnect(HWND hwnd, const std::wstring& ca_filename, fw::PortalClient* portal):
	SyncProc(hwnd, AsyncMessage::ConnectedEvent),
	_ca_filename(ca_filename),
	_portal(portal)
{
	DEBUG_CTOR(_logger, "SyncConnect");
}


SyncConnect::~SyncConnect()
{
	DEBUG_DTOR(_logger, "SyncConnect");
}


void SyncConnect::showErrorMessageDialog(const char* message) const
{
	AsyncMessage::ShowErrorMessageDialogRequest.send(_hwnd, (void *)message);
}


bool SyncConnect::procedure()
{
	DEBUG_ENTER(_logger, "SyncConnect", "procedure");

	// .. Check which CA file to use by the client 
	tools::Path ca_path;

	if (_ca_filename.length() == 0) {
		// no CA file specified, use the default file located in the application folder
		ca_path = tools::Path(tools::Path::get_module_path().folder(), L"fortirdp.crt");

	} else {
		// use the CA file specified in the command line
		ca_path = tools::Path(_ca_filename);

		// if no path specified, we assume that the .crt file is located next to the .exe
		if (ca_path.folder().empty()) {
			ca_path = tools::Path(tools::Path::get_module_path().folder(), _ca_filename);
		}
	}

	// .. Is the CA file available ?
	const bool ca_found = tools::file_exists(ca_path.to_string());

	if (!ca_found) {
		// log an error
		const std::string compacted = tools::wstr2str(ca_path.compact(50));
		_logger->error("ERROR: can't find CA file %s", compacted.c_str());
	}

	if (ca_found)
		_portal->set_ca_file(ca_path);
	else
		_portal->set_ca_crt(nullptr);

	// Open the https connection
	fw::confirm_crt_fn confirm_crt_callback = [this](const mbedtls_x509_crt* crt, int status) {
		return confirm_certificate(crt, status);
	};
	int rc = _portal->open(confirm_crt_callback);

	if (rc) {
		// report errors.
		if (rc != fw::portal_err::CERT_UNTRUSTED) {
			showErrorMessageDialog("Connection error");
		}

	} else {
		auto ask_credential_callback = [this](fw::Credential& credential) {
			return ask_credential(credential);
		};

		auto ask_code_callback = [this](fw::Code2FA& code) {
			return ask_code(code);
		};

		// loop while user enter wrong credentials
		do {
			rc = _portal->login(ask_credential_callback, ask_code_callback);
			if (rc) {
				// report errors.
				if (rc != fw::portal_err::LOGIN_CANCELLED) {
					showErrorMessageDialog("Login error");
				}
			}
		} while (rc == fw::portal_err::ACCESS_DENIED);

		if (!rc) {
			fw::PortalInfo portal_info;
			if (_portal->get_info(portal_info)) {
				_logger->info(">> portal info user=%s group=%s",
					portal_info.user.c_str(),
					portal_info.group.c_str());
			}

			fw::SslvpnConfig vpn_config;
			if (_portal->get_config(vpn_config)) {
				if (!_portal->start_tunnel_mode())
					showErrorMessageDialog("Open tunnel error");
			}
		}
	}

	return rc == 0;
}


bool SyncConnect::confirm_certificate(const mbedtls_x509_crt* crt, int status)
{
	char buffer[4096];

	mbedtls_x509_crt_verify_info(buffer, sizeof(buffer), " ** ", status);
	_logger->info(buffer);

	std::string message("The security certificate is not valid.\n");
	message.append(buffer);
	message.append("\n");

	message.append("Security certificate problems may indicate an attempt to intercept any data including passwords you send to the firewall.\n");
	message.append("\n"); 
	message.append("Do you want to proceed ?");

	return AsyncMessage::ShowInvalidCertificateDialogRequest.send(_hwnd, (void *) message.c_str()) == TRUE;
}


bool SyncConnect::ask_credential(fw::Credential& credential)
{
	return AsyncMessage::ShowAskCredentialDialogRequest.send(_hwnd, (void *)&credential) == TRUE;
}


bool SyncConnect::ask_code(fw::Code2FA& code2fa)
{
	return AsyncMessage::ShowAskCodeDialogRequest.send(_hwnd, (void *)&code2fa) == TRUE;
}
