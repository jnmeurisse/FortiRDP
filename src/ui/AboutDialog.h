/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <Windows.h>
#include "ModalDialog.h"

/**
* The FortiRDP about dialog
*/
class AboutDialog final : public ModalDialog
{
public:
	explicit AboutDialog(HINSTANCE hInstance, HWND hParent);
	virtual ~AboutDialog();

private:
	HFONT _hFont;

	virtual INT_PTR onCreateDialogMessage(WPARAM wParam, LPARAM lParam) override;
	virtual INT_PTR onButtonClick(int cid, LPARAM lParam) override;
};

