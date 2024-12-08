/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <Windows.h>
#include <wil/com.h>
#include <WebView2.h>
#include "ui/ModalDialog.h"

namespace ui {

	class SamlAuthDialog final : public ModalDialog
	{
	public:
		explicit SamlAuthDialog(HINSTANCE hInstance, HWND hParent);
		~SamlAuthDialog();

	private:
		bool _can_close;
		wil::com_ptr<ICoreWebView2Controller> _web_controller;

		virtual INT_PTR onCreateDialogMessage(WPARAM wParam, LPARAM lParam) override;
		virtual INT_PTR onSysCommandMessage(WPARAM wParam, LPARAM lParam);

		HRESULT onWebViewNavigationCompleted(ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args);
	};

}