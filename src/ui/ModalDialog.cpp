/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "ModalDialog.h"

namespace ui {

	ModalDialog::ModalDialog(HINSTANCE hInstance, HWND hParent, int idd) :
		Dialog(hInstance, hParent, idd)
	{
	}


	bool ModalDialog::close_dialog(INT_PTR result)
	{
		return ::EndDialog(window_handle(), result) != 0;
	}


	INT_PTR ModalDialog::show_modal()
	{
		return Dialog::create_modal_dialog();
	}

}
