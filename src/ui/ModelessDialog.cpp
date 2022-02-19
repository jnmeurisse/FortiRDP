/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "ui/ModelessDialog.h"
#include "tools/SysUtil.h"

ModelessDialog::ModelessDialog(HINSTANCE hInstance, HWND hParent, int idd) :
	Dialog(hInstance, hParent, idd)
{
	Dialog::create_modeless_dialog();
}


