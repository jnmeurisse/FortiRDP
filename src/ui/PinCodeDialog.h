/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <Windows.h>
#include <string>
#include "ui/ModalDialog.h"

namespace ui {

	class PinCodeDialog final : public ModalDialog
	{
	public:
		explicit PinCodeDialog(HINSTANCE hInstance, HWND hParent);
		~PinCodeDialog();

		/* Initializes the text to show in the ask code dialog
		*/
		void setText(const std::wstring& text);

		/* Returns the code specified by the user
		*/
		const std::wstring& getCode() const;

	private:
		std::wstring _text;
		std::wstring _code;

		virtual INT_PTR onCreateDialogMessage(WPARAM wParam, LPARAM lParam) override;
		virtual INT_PTR onButtonClick(int cid, LPARAM lParam) override;
		virtual INT_PTR onTextChange(int idc, LPARAM lParam) override;
	};

}
