/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "SamlDialog.h"

#include <objbase.h>
#include <wrl.h>
#include "resources/resource.h"
#include "tools/ErrUtil.h"


namespace ui {
	using namespace Microsoft::WRL;

	SamlAuthDialog::SamlAuthDialog(HINSTANCE hInstance, HWND hParent) :
		ModalDialog(hInstance, hParent, IDD_SAMLAUTH_DIALOG),
		_web_controller(),
		_can_close(false)
	{
		HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		if (FAILED(hr))
			throw tools::win_err(hr);
	}

	
	SamlAuthDialog::~SamlAuthDialog()
	{
		::CoUninitialize();
	}

	
	INT_PTR SamlAuthDialog::onCreateDialogMessage(WPARAM wParam, LPARAM lParam)
	{
		auto web_core_created = [this](HRESULT result, ICoreWebView2Controller* controller) {
			wil::com_ptr<ICoreWebView2> web_view;
			_web_controller = controller;
			_web_controller->get_CoreWebView2(&web_view);

			// Set up WebView
			_web_controller->put_Bounds(get_client_bounds());

			wil::com_ptr<ICoreWebView2Settings> settings;
			web_view->get_Settings(&settings);
			settings->put_AreDefaultContextMenusEnabled(false);
			settings->put_IsStatusBarEnabled(false);
			settings->put_AreDevToolsEnabled(false);
			settings->put_IsZoomControlEnabled(false);

			// Install a navigation callback
			EventRegistrationToken navCompletedToken;
			web_view->add_NavigationCompleted(
				Callback<ICoreWebView2NavigationCompletedEventHandler>(
					this,
					&SamlAuthDialog::onWebViewNavigationCompleted
				).Get(),
				&navCompletedToken
			);

			// Install a new window callback that blocks new windows
			EventRegistrationToken newWindowToken;
			web_view->add_NewWindowRequested(
				Callback<ICoreWebView2NewWindowRequestedEventHandler>(
					[this](ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args) {
						args->put_Handled(true);
						return S_OK;
					}
				).Get(),
				&newWindowToken
			);

			// Navigate to URL
			web_view->Navigate(L"https://x.x.com");

			return S_OK;
		};

		auto web_env_created = [this, web_core_created](HRESULT result, ICoreWebView2Environment* env) {
			// Create WebView2 controller
			env->CreateCoreWebView2Controller(
				window_handle(),
				Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(web_core_created).Get()
			);

			return S_OK;
		};


		HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
			nullptr,
			nullptr,
			nullptr,
			Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(web_env_created).Get()
		);

		return SUCCEEDED(hr);
	}


	INT_PTR SamlAuthDialog::onSysCommandMessage(WPARAM wParam, LPARAM lParam)
	{
		INT_PTR rc = TRUE;

		switch (wParam) {
		case SC_CLOSE:
			if (_can_close) {
				close(false);
			}
			break;

		default:
			rc = FALSE;
		}

		return rc;
	}



	HRESULT SamlAuthDialog::onWebViewNavigationCompleted(ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args)
	{
		_can_close = true;
		return S_OK;
	}

}
