/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "OptionsDialog.h"

#include "tools/StrUtil.h"
#include "resources/resource.h"


OptionsDialog::OptionsDialog(HINSTANCE hInstance, HWND hParent) :
	ModalDialog(hInstance, hParent, IDD_PARAMS_DIALOG)
{
}


OptionsDialog::~OptionsDialog()
{
}


INT_PTR OptionsDialog::onCreateDialogMessage(WPARAM wParam, LPARAM lParam)
{
	set_checkbox_state(IDC_CHECK_FULLSCREEN, full_screen);
	set_control_enable(IDC_CHECK_FULLSCREEN, full_screen_updatable);

	set_control_enable(IDC_SCREEN_HEIGHT, !full_screen && screen_size_updatable);
	set_control_textlen(IDC_SCREEN_HEIGHT, 5);
	if (screen_size.height > 0)
		set_control_text(IDC_SCREEN_HEIGHT, std::to_wstring(screen_size.height));

	set_control_enable(IDC_SCREEN_WIDTH, !full_screen  && screen_size_updatable);
	set_control_textlen(IDC_SCREEN_WIDTH, 5);
	if (screen_size.width > 0)
		set_control_text(IDC_SCREEN_WIDTH, std::to_wstring(screen_size.width));

	set_checkbox_state(IDC_CHECK_CLEAR_USERNAME, clear_rdp_username);
	set_control_enable(IDC_CHECK_CLEAR_USERNAME, clear_rdp_username_updatable);

	set_checkbox_state(IDC_CHECK_SPAN_MODE, span_mode);
	set_control_enable(IDC_CHECK_SPAN_MODE, span_mode_updatable);

	set_checkbox_state(IDC_CHECK_MULTIMON_MODE, multimon_mode);
	set_control_enable(IDC_CHECK_MULTIMON_MODE, multimon_mode_updatable);

	set_checkbox_state(IDC_CHECK_ADMIN_CONSOLE, admin_console);
	set_control_enable(IDC_CHECK_ADMIN_CONSOLE, admin_console_updatable);

	set_checkbox_state(IDC_CHECK_RDPFILE, rdpfile_mode);
	set_control_enable(IDC_CHECK_RDPFILE, rdpfile_updatable);

	set_control_text(IDC_EDIT_RDPFILE, rdp_filename);
	set_control_textlen(IDC_EDIT_RDPFILE, FILENAME_MAX);
	set_control_enable(IDC_EDIT_RDPFILE, rdpfile_updatable && rdpfile_mode);
	set_control_enable(IDC_SELECT_RPDFILE, rdpfile_updatable && rdpfile_mode);

	center_window();

	return TRUE;
}


INT_PTR OptionsDialog::onButtonClick(int cid, LPARAM lParam)
{
	INT_PTR rc = FALSE;
	std::wstring filename;

	switch (cid) {
	case IDOK:
		full_screen = get_checkbox_state(IDC_CHECK_FULLSCREEN);
		if (!full_screen && screen_size_updatable) {
			if (!tools::str2i(get_control_text(IDC_SCREEN_WIDTH), screen_size.width)
				|| screen_size.width < 0 || screen_size.width > ScreenSize::max_width) {
				set_focus(IDC_SCREEN_WIDTH);
				show_message_box(L"Invalid screen width", MB_OK | MB_ICONERROR);

				break;
			}

			if (!tools::str2i(get_control_text(IDC_SCREEN_HEIGHT), screen_size.height)
				|| screen_size.height < 0 || screen_size.height > ScreenSize::max_height) {
				set_focus(IDC_SCREEN_HEIGHT);
				show_message_box(L"Invalid screen height", MB_OK | MB_ICONERROR);

				break;
			}

		}

		clear_rdp_username = get_checkbox_state(IDC_CHECK_CLEAR_USERNAME);
		span_mode = get_checkbox_state(IDC_CHECK_SPAN_MODE);
		multimon_mode = get_checkbox_state(IDC_CHECK_MULTIMON_MODE);
		admin_console = get_checkbox_state(IDC_CHECK_ADMIN_CONSOLE);
		rdpfile_mode = get_checkbox_state(IDC_CHECK_RDPFILE);
		if (rdpfile_mode)
			rdp_filename = get_control_text(IDC_EDIT_RDPFILE);

		close(TRUE);
		break;

	case IDCANCEL:
		close(FALSE);
		break;

	case IDC_CHECK_RDPFILE:
		set_control_enable(IDC_EDIT_RDPFILE, get_checkbox_state(IDC_CHECK_RDPFILE));
		set_control_enable(IDC_SELECT_RPDFILE, get_checkbox_state(IDC_CHECK_RDPFILE));
		break;

	case IDC_SELECT_RPDFILE:
		filename = get_control_text(IDC_EDIT_RDPFILE);
		if (select_file(filename))
			set_control_text(IDC_EDIT_RDPFILE, filename);
		break;

	case IDC_CHECK_FULLSCREEN:
		set_control_enable(IDC_SCREEN_HEIGHT, !get_checkbox_state(IDC_CHECK_FULLSCREEN) && screen_size_updatable);
		set_control_enable(IDC_SCREEN_WIDTH, !get_checkbox_state(IDC_CHECK_FULLSCREEN) && screen_size_updatable);
		break;

	default:
		rc = TRUE;
		break;
	}

	return rc;
}

bool OptionsDialog::select_file(std::wstring& filename)
{
	wchar_t tmp[MAX_PATH] = { 0 };
	OPENFILENAME ofn = { 0 };
	
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = window_handle(); 
	ofn.lpstrFilter = L"RDP Files\0*.rdp\0\0";
	ofn.lpstrFile = tmp;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = L"Select a rdp File";
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_ENABLESIZING;

	if (::GetOpenFileName(&ofn)) {
		filename = ofn.lpstrFile;
		return true;
	} else {
		return false;
	}
}

