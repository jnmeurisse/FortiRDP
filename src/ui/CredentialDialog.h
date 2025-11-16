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

	class CredentialDialog final : public ModalDialog
	{
	public:
		explicit CredentialDialog(HINSTANCE hInstance, HWND hParent);
		virtual ~CredentialDialog() override;

		/**
		 * Initializes the information label.
		*/
		void setText(const std::wstring& text);

		/**
		 * Initializes the user name.
		*/
		void setUsername(const std::wstring& username);

		/**
		 * Returns the user name from the login dialog.
		*/
		const std::wstring& getUsername() const;

		/**
		 * Returns the password from the login dialog.
		*/
		const std::wstring& getPassword() const;

	private:
		std::wstring _text;
		std::wstring _username;
		std::wstring _password;

		virtual INT_PTR onCreateDialogMessage(WPARAM wParam, LPARAM lParam) override;
		virtual INT_PTR onButtonClick(int control_id, LPARAM lParam) override;
	};

}
