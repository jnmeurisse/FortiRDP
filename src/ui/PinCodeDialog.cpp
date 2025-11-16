/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "PinCodeDialog.h"

#include "resources\resource.h"

namespace ui {

	PinCodeDialog::PinCodeDialog(HINSTANCE hInstance, HWND hParent) :
		ModalDialog(hInstance, hParent, IDD_CODE_DIALOG)
	{
	}


	PinCodeDialog::~PinCodeDialog()
	{
	}


	void PinCodeDialog::setText(const std::wstring& info)
	{
		_text = info;
	}


	const std::wstring& PinCodeDialog::getCode() const
	{
		return _code;
	}


	INT_PTR PinCodeDialog::onCreateDialogMessage(WPARAM wParam, LPARAM lParam)
	{
		LPARAM_UNUSED();

		set_control_textlen(IDC_CODE, 128);
		set_control_text(IDC_CODE_LABEL, _text);
		set_control_enable(IDOK, FALSE);

		center_window();
		

		if (::GetDlgCtrlID((HWND)wParam) != IDC_CODE) {
			set_focus(IDC_CODE);
			return FALSE;
		}

		return TRUE;
	}


	INT_PTR PinCodeDialog::onButtonClick(int control_id, LPARAM lParam)
	{
		LPARAM_UNUSED();

		INT_PTR rc = FALSE;

		switch (control_id) {
		case IDOK:
			_code = get_control_text(IDC_CODE);

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


	INT_PTR PinCodeDialog::onTextChange(int control_id, LPARAM lParam)
	{
		LPARAM_UNUSED();

		INT_PTR rc = FALSE;
		if (control_id == IDC_CODE)
			set_control_enable(IDOK, get_control_text(IDC_CODE).size() > 0);
		return rc;
	}

}
