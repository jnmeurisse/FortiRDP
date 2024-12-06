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


/**
* A modal dialog base class. The caller must call showModal to display the
* dialog. The derived class must implement a onCreateDialogMessage to initialize
* the dialog.
*/
class ModalDialog : public Dialog
{
public:
	explicit ModalDialog(HINSTANCE hInstance, HWND hParent, int idd);

	/* destroys this modal dialog.
	*/
	void close(INT_PTR result);

	INT_PTR showModal();
};