/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include <Windows.h>
#include <wil/com.h>
#include <WebView2.h>
#include "fw/AuthTypes.h"
#include "http/Url.h"
#include "http/Cookies.h"
#include "tools/Logger.h"
#include "ui/ModalDialog.h"


namespace ui {

	enum class saml_err {
		NONE = 0,
		WEBVIEW_ERROR,
		COMM_ERROR,
		CERT_UNTRUSTED,
		HTTP_ERROR,
		ACCESS_DENIED,
		LOGIN_CANCELLED
	};


	class SamlAuthDialog final : public ModalDialog
	{
	public:
		explicit SamlAuthDialog(HINSTANCE hInstance, HWND hParent, fw::AuthSamlInfo* pSamlInfo);
		virtual ~SamlAuthDialog() override;

		ui::saml_err get_saml_error() const;

	private:
		// The class name.
		static const char* __class__;

		// The application logger.
		tools::Logger* const _logger;

		// SAML authentication configuration.
		fw::AuthSamlInfo& _saml_auth_info;

		// Set to true when this dialog can be closed.
		bool _can_close;

		// Last error code.
		ui::saml_err _last_saml_error;

		// A reference to the webview controller.
		wil::com_ptr<ICoreWebView2Controller> _web_controller;

		// A reference to the cookie manager.
		wil::com_ptr <ICoreWebView2CookieManager> _web_cookie_manager;

		virtual INT_PTR onCreateDialogMessage(WPARAM wParam, LPARAM lParam) override;
		virtual INT_PTR onSysCommandMessage(WPARAM wParam, LPARAM lParam) override;

		HRESULT onWebViewNavigationStarting(ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args);
		HRESULT onWebViewNavigationCompleted(ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args);
		HRESULT onSourceChanged(ICoreWebView2* sender, ICoreWebView2SourceChangedEventArgs* args);
		HRESULT onWebViewServerCertificateErrorDetected(ICoreWebView2* sender, ICoreWebView2ServerCertificateErrorDetectedEventArgs* args);
		HRESULT onCookiesAvailable(HRESULT result, ICoreWebView2CookieList* list);
	};

}