/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "LoginDialog.h"

#include "resources/resource.h"

namespace ui {

	LoginDialog::LoginDialog(HINSTANCE hInstance, HWND hParent) :
		ModalDialog(hInstance, hParent, IDD_LOGIN_DIALOG),
		credential(),
		save_password(false),
		_password_updated(false)
	{
	}


	LoginDialog::~LoginDialog()
	{
	}


	INT_PTR LoginDialog::onCreateDialogMessage(WPARAM wParam, LPARAM lParam)
	{
		LPARAM_UNUSED();

		set_control_textlen(IDC_LOGIN_PROMPT, 128);
		set_control_textlen(IDC_USERNAME, 128);

		set_control_text(IDC_LOGIN_PROMPT, header);
		set_control_text(IDC_USERNAME, credential.username);
		set_checkbox_state(IDC_CHECK_SAVE_PASSWORD, save_password);
		if (save_password)
			set_control_text(IDC_PASSWORD, L"********");

		_password_updated = false;
		center_window();

		const HWND control = reinterpret_cast<HWND>(wParam);
		if (::GetDlgCtrlID(control) != IDC_PASSWORD) {
			set_focus(IDC_PASSWORD);
			return FALSE;
		}

		return TRUE;
	}


	INT_PTR LoginDialog::onTextChange(int control_id, LPARAM lParam)
	{
		LPARAM_UNUSED();

		INT_PTR rc = FALSE;
		if (control_id == IDC_PASSWORD)
			_password_updated = true;

		return rc;
	}


	INT_PTR LoginDialog::onButtonClick(int control_id, LPARAM lParam)
	{
		LPARAM_UNUSED();

		INT_PTR rc = FALSE;

		switch (control_id) {
		case IDOK:
			credential.username = get_control_text(IDC_USERNAME);
			if (_password_updated)
				credential.password = get_control_text(IDC_PASSWORD);
			save_password = get_checkbox_state(IDC_CHECK_SAVE_PASSWORD);

			close_dialog(TRUE);
			break;

		case IDCANCEL:
			close_dialog(FALSE);
			break;

		default:
			rc = TRUE;
			break;
		}

		return rc;
	}

}
