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


	void ModalDialog::close(INT_PTR result)
	{
		::EndDialog(window_handle(), result);
	}


	INT_PTR ModalDialog::show_modal()
	{
		return Dialog::create_modal_dialog();
	}

}
