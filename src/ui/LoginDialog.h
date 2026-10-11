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

#include "ui/ModalDialog.h"
#include "util/Credential.h"

namespace ui {

	class LoginDialog final : public ModalDialog
	{
	public:
		explicit LoginDialog(HINSTANCE hInstance, HWND hParent);
		~LoginDialog() override;

		/* Login dialog header.
		*/
		std::wstring header;

		/* User's credential.
		*/
		utl::Credential credential;

		/* Save password flag.
		*/
		bool save_password;

	private:
		/* set to true if the password was updated.
		*/
		bool _password_updated;

		INT_PTR onCreateDialogMessage(WPARAM wParam, LPARAM lParam) override;
		INT_PTR onTextChange(int control_id, LPARAM lParam) override;
		INT_PTR onButtonClick(int control_id, LPARAM lParam) override;
	};

}
