/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "CredentialDialog.h"

#include "resources/resource.h"

namespace ui {

	CredentialDialog::CredentialDialog(HINSTANCE hInstance, HWND hParent) :
		ModalDialog(hInstance, hParent, IDD_CREDENTIAL_DIALOG)
	{
	}


	CredentialDialog::~CredentialDialog()
	{
	}


	void CredentialDialog::setText(const std::wstring& info)
	{
		_text = info;
	}


	void CredentialDialog::setUsername(const std::wstring& username)
	{
		_username = username;
	}


	const std::wstring& CredentialDialog::getUsername() const
	{
		return _username;
	}


	const std::wstring& CredentialDialog::getPassword() const
	{
		return _password;
	}


	INT_PTR CredentialDialog::onCreateDialogMessage(WPARAM wParam, LPARAM lParam)
	{
		set_control_textlen(IDC_LOGIN_INFO, 128);
		set_control_textlen(IDC_USERNAME, 128);

		set_control_text(IDC_LOGIN_INFO, _text);
		set_control_text(IDC_USERNAME, _username);

		center_window();

		if (::GetDlgCtrlID((HWND)wParam) != IDC_PASSWORD) {
			set_focus(IDC_PASSWORD);
			return FALSE;
		}

		return TRUE;
	}


	INT_PTR CredentialDialog::onButtonClick(int cid, LPARAM lParam)
	{
		INT_PTR rc = FALSE;

		switch (cid) {
		case IDOK:
			_username = get_control_text(IDC_USERNAME);
			_password = get_control_text(IDC_PASSWORD);

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
