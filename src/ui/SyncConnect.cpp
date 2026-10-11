/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "SyncConnect.h"

#include <array>
#include <string>


namespace ui {
	SyncConnect::SyncConnect(HWND hwnd, fw::AuthMethod auth_method, fw::FirewallClient& portal_client) :
		SyncProc(hwnd, AsyncMessage::ConnectedEvent.get()),
		_auth_method(auth_method),
		_portal_client(portal_client)
	{
		DEBUG_CTOR(_logger);

	}


	SyncConnect::~SyncConnect()
	{
		DEBUG_DTOR(_logger);
	}


	void SyncConnect::showErrorMessageDialog(const wchar_t* message) const
	{
		AsyncMessage::ShowErrorMessageDialogRequest->send_message(_hwnd, message);
	}


	bool SyncConnect::confirm_certificate(const mbedtls_x509_crt* crt, int status)
	{
		std::array<char, 4096> buffer;
		std::string message("The security certificate is not valid.\n");

		if (mbedtls_x509_crt_verify_info(buffer.data(), buffer.size(), " * ", status) > 0) {
			_logger.info(buffer.data());

			message.append(buffer.data());
			message.append("\n");
		}

		message.append("Security certificate problems may indicate an attempt to "
			"intercept any data including passwords you send to the firewall.\n");
		message.append("\n");
		message.append("Do you want to proceed ?");

		return AsyncMessage::ShowInvalidCertificateDialogRequest->send_message(_hwnd, message.c_str()) == TRUE;
	}


	bool SyncConnect::ask_credentials(fw::AuthCredentialRequest& credential)
	{
		return AsyncMessage::ShowCredentialDialogRequest->send_message(_hwnd, &credential) == TRUE;
	}


	bool SyncConnect::ask_pincode(fw::AuthCodeRequest& code2fa)
	{
		return AsyncMessage::ShowPinCodeDialogRequest->send_message(_hwnd, &code2fa) == TRUE;
	}


	bool SyncConnect::ask_saml_auth(fw::AuthSamlInfo& saml_info)
	{
		return AsyncMessage::ShowSamlAuthDialogRequest->send_message(_hwnd, &saml_info) == TRUE;
	}


	bool SyncConnect::procedure()
	{
		DEBUG_ENTER(_logger);

		bool connected = false;

		// Open an HTTPS connection with the firewall portal.
		{
			fw::confirm_crt_fn confirm_crt_callback = [this](const mbedtls_x509_crt* crt, int status) {
				return confirm_certificate(crt, status);
			};
			const fw::portal_err rc = _portal_client.open(confirm_crt_callback);

			if (rc != fw::portal_err::NONE) {
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
					auto ask_credential_callback = [this](fw::AuthCredentialRequest& credential) {
						return ask_credentials(credential);
					};

					auto ask_pincode_callback = [this](fw::AuthCodeRequest& code) {
						return ask_pincode(code);
					};

					// Loop while user enter wrong credentials.
					fw::portal_err rc;
					do {
						rc = _portal_client.login_basic(ask_credential_callback, ask_pincode_callback);
						if (rc != fw::portal_err::NONE) {
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

					fw::portal_err rc = _portal_client.login_saml(ask_samlauth_callback);
					if (rc != fw::portal_err::NONE) {
						showErrorMessageDialog(L"Login error");
					}
				}
				break;
			}

			if (_portal_client.is_authenticated()) {
				fw::PortalInfo portal_info;
				if (!_portal_client.get_info(portal_info)) {
					showErrorMessageDialog(L"Open tunnel error");
					goto exit;
				}

				fw::SslvpnConfig sslvpn_config;
				if (!_portal_client.get_config(sslvpn_config)) {
					showErrorMessageDialog(L"Open tunnel error");
					goto exit;
				}

				_logger.info(">> portal info");
				_logger.info("     user: %s", portal_info.user.c_str());
				_logger.info("     group: %s", portal_info.group.c_str());
			}
		}

	exit:
		connected = connected &&
			_portal_client.is_connected() &&
			_portal_client.is_authenticated();

		if (!connected)
			_logger.info(">> disconnected");

		return connected;
	}

	const char* SyncConnect::__class__ = "SyncConnect";
}
