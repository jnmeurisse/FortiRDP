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
		explicit SamlAuthDialog(HINSTANCE hInstance, HWND hParent);
		~SamlAuthDialog();

		void set_service_provider_url(const http::Url& url);
		const http::Url& get_service_provider_url() const;

		void set_service_provider_crt(const std::string& crt);
		const std::string& get_service_provider_crt() const;

		const http::Cookies& get_service_provider_cookies() const;

		ui::saml_err get_saml_error() const;

	private:
		// the application logger
		tools::Logger* const _logger;

		// The Service Provider URL
		http::Url _service_provider_url;

		// The Service Provider certificate
		std::string _service_provider_crt;

		// The Service Provider auth cookie
		http::Cookies _service_provider_cookies;

		// Set to true when this dialog can be closed.
		bool _can_close;

		//
		ui::saml_err _last_saml_error;

		// A reference to the webview controller
		wil::com_ptr<ICoreWebView2Controller> _web_controller;

		// A reference to the cookie manager
		wil::com_ptr <ICoreWebView2CookieManager> _web_cookie_manager;

		virtual INT_PTR onCreateDialogMessage(WPARAM wParam, LPARAM lParam) override;
		virtual INT_PTR onSysCommandMessage(WPARAM wParam, LPARAM lParam);

		HRESULT onWebViewNavigationCompleted(ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args);
		HRESULT onSourceChanged(ICoreWebView2* sender, ICoreWebView2SourceChangedEventArgs* args);
		HRESULT onWebViewServerCertificateErrorDetected(ICoreWebView2* sender, ICoreWebView2ServerCertificateErrorDetectedEventArgs* args);
		HRESULT onCookiesAvailable(HRESULT result, ICoreWebView2CookieList* list);
	};

}