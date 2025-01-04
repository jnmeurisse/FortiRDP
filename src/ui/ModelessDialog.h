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
	/*
	* A modeless dialog base class
	*/
	class ModelessDialog : public Dialog
	{
	public:
		explicit ModelessDialog(HINSTANCE hInstance, HWND hParent, int idd);

	};

}
