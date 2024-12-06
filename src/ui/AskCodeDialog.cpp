/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "AskCodeDialog.h"

#include "resources\resource.h"

namespace ui {

	AskCodeDialog::AskCodeDialog(HINSTANCE hInstance, HWND hParent) :
		ModalDialog(hInstance, hParent, IDD_CODE_DIALOG)
	{
	}


	AskCodeDialog::~AskCodeDialog()
	{
	}


	void AskCodeDialog::setText(const std::wstring& info)
	{
		_text = info;
	}


	const std::wstring& AskCodeDialog::getCode() const
	{
		return _code;
	}


	INT_PTR AskCodeDialog::onCreateDialogMessage(WPARAM wParam, LPARAM lParam)
	{
		set_control_textlen(IDC_CODE, 128);
		set_control_text(IDC_CODE_LABEL, _text);

		center_window();

		if (GetDlgCtrlID((HWND)wParam) != IDC_CODE) {
			set_focus(IDC_CODE);
			return FALSE;
		}

		return TRUE;
	}


	INT_PTR AskCodeDialog::onButtonClick(int cid, LPARAM lParam)
	{
		INT_PTR rc = FALSE;

		switch (cid) {
		case IDOK:
			_code = get_control_text(IDC_CODE);

			close(TRUE);
			break;

		case IDCANCEL:
			close(FALSE);
			break;

		default:
			rc = TRUE;
			break;
		}

		return rc;
	}

}
