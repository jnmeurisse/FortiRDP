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
#include "fw/AuthTypes.h"
#include "ui/ModalDialog.h"
#include "ui/ScreenSize.h"

namespace ui {

	class OptionsDialog final : public ModalDialog
	{
	public:
		explicit OptionsDialog(HINSTANCE hInstance, HWND hParent);
		virtual ~OptionsDialog();

		fw::AuthMethod auth_method = fw::AuthMethod::BASIC;

		bool full_screen = false;
		bool full_screen_updatable = false;

		bool clear_rdp_username = false;
		bool clear_rdp_username_updatable = false;

		bool span_mode = false;
		bool span_mode_updatable = false;

		bool multimon_mode = false;
		bool multimon_mode_updatable = false;

		bool admin_console = false;
		bool admin_console_updatable = false;

		ScreenSize screen_size = { 0, 0 };
		bool screen_size_updatable = false;

		bool rdpfile_mode = false;
		bool rdpfile_updatable = false;

		std::wstring rdp_filename;

	private:
		virtual INT_PTR onCreateDialogMessage(WPARAM wParam, LPARAM lParam) override;
		virtual INT_PTR onButtonClick(int cid, LPARAM lParam) override;

		bool select_file(std::wstring& filename);
	};

}

