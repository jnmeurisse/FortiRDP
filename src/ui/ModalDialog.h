/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/

#pragma once

#include <Windows.h>
#include "ui/Dialog.h"

namespace ui {

	/**
	* A modal dialog base class. The caller must call show_modal to display the
	* dialog. The derived class must implement a onCreateDialogMessage to initialize
	* the dialog.
	*/
	class ModalDialog : public Dialog
	{
	public:
		explicit ModalDialog(HINSTANCE hInstance, HWND hParent, int idd);

		/**
		 * Closes this modal dialog.
		 * 
		*/
		bool close_dialog(INT_PTR result);

		/**
		 * Creates and shows this modal dialog.
		 *
		 * The function returns the value passed in the call to close_dialog.
		*/
		INT_PTR show_modal();
	};

}
