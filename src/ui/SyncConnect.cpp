/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "SyncConnect.h"

#include "tools/Path.h"
#include "tools/SysUtil.h"


namespace ui {
	SyncConnect::SyncConnect(HWND hwnd, fw::AuthMethod auth_method, fw::PortalClient& portal) :
		SyncProc(hwnd, AsyncMessage::ConnectedEvent),
		_auth_method(auth_method),
		_portal(portal)
	{
		DEBUG_CTOR(_logger, "SyncConnect");

	}


	SyncConnect::~SyncConnect()
	{
		DEBUG_DTOR(_logger, "SyncConnect");
	}


	void SyncConnect::showErrorMessageDialog(const wchar_t* message) const
	{
		AsyncMessage::ShowErrorMessageDialogRequest.send(_hwnd, (void*)message);
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

		return AsyncMessage::ShowInvalidCertificateDialogRequest.send(_hwnd, (void*)message.c_str()) == TRUE;
	}


	bool SyncConnect::ask_credentials(fw::AuthCredentials& credential)
	{
		return AsyncMessage::ShowCredentialsDialogRequest.send(_hwnd, (void*)&credential) == TRUE;
	}


	bool SyncConnect::ask_pincode(fw::AuthCode& code2fa)
	{
		return AsyncMessage::ShowPinCodeDialogRequest.send(_hwnd, (void*)&code2fa) == TRUE;
	}


	bool SyncConnect::ask_saml_auth(fw::AuthSamlInfo& saml_info)
	{
		return AsyncMessage::ShowSamlAuthDialogRequest.send(_hwnd, (void*)&saml_info) == TRUE;
	}


	bool SyncConnect::procedure()
	{
		DEBUG_ENTER(_logger, "SyncConnect", "procedure");

		bool connected = false;

		// Open an https connection with the firewall portal.
		{
			fw::confirm_crt_fn confirm_crt_callback = [this](const mbedtls_x509_crt* crt, int status) {
				return confirm_certificate(crt, status);
			};
			fw::portal_err rc = _portal.open(confirm_crt_callback);

			if (rc != fw::portal_err::NONE) {
				// report errors.
				if (rc != fw::portal_err::CERT_UNTRUSTED) {
					showErrorMessageDialog(L"Connection error");
				}
			}
			else
				connected = true;
		}

		// Start the authentication procedure.
		if (connected) {
			switch (_auth_method) {
				case fw::AuthMethod::BASIC:
				case fw::AuthMethod::DEFAULT:
				{
					auto ask_credentials_callback = [this](fw::AuthCredentials& credential) {
						return ask_credentials(credential);
					};

					auto ask_pincode_callback = [this](fw::AuthCode& code) {
						return ask_pincode(code);
					};

					// loop while user enter wrong credentials
					fw::portal_err rc;
					do {
						rc = _portal.login_basic(ask_credentials_callback, ask_pincode_callback);
						if (rc != fw::portal_err::NONE) {
							// report errors.
							if (rc != fw::portal_err::LOGIN_CANCELLED) {
								showErrorMessageDialog(L"Login error");
							}
						}
					} while (rc == fw::portal_err::ACCESS_DENIED);
				}
				break;

				case fw::AuthMethod::CERTIFICATE:
					break;

				case fw::AuthMethod::SAML:
				{
					auto ask_samlauth_callback = [this](fw::AuthSamlInfo& saml_info) {
						return ask_saml_auth(saml_info);
					};

					fw::portal_err rc = _portal.login_saml(ask_samlauth_callback);
					if (rc != fw::portal_err::NONE) {
						showErrorMessageDialog(L"Login error");
					}
				}
				break;
			}

			if (_portal.is_authenticated()) {
				fw::PortalInfo portal_info;
				if (!_portal.get_info(portal_info)) {
					showErrorMessageDialog(L"Open tunnel error");
				}

				fw::SslvpnConfig sslvpn_config;
				if (!_portal.get_config(sslvpn_config)) {
					showErrorMessageDialog(L"Open tunnel error");
				}

				_logger->info(">> portal info user=%s group=%s address=%s",
					portal_info.user.c_str(),
					portal_info.group.c_str(),
					sslvpn_config.local_addr.c_str());
			}
		}

		return connected && _portal.is_connected() && _portal.is_authenticated();
	}

}
