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

		/**
		 * Initializes the information text.
		*/
		void setText(const std::wstring& text);

		/**
		 * Returns the pin code.
		*/
		const std::wstring& getCode() const;

	private:
		std::wstring _text;
		std::wstring _code;

		virtual INT_PTR onCreateDialogMessage(WPARAM wParam, LPARAM lParam) override;
		virtual INT_PTR onButtonClick(int control_id, LPARAM lParam) override;
		virtual INT_PTR onTextChange(int control_id, LPARAM lParam) override;
	};

}
