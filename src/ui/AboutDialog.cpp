/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "AboutDialog.h"

#include <mbedtls/version.h>
#include <lwip/init.h>
#include "tools/SysUtil.h"
#include "tools/StrUtil.h"
#include "tools/Path.h"
#include "resources/resource.h"

namespace ui {

	AboutDialog::AboutDialog(HINSTANCE hInstance, HWND hParent) :
		ModalDialog(hInstance, hParent, IDD_ABOUT_DIALOG)
	{
		_hFont = create_font(10, L"Tahoma");
	}


	AboutDialog::~AboutDialog()
	{
		::DeleteObject(_hFont);
	}


	INT_PTR AboutDialog::onCreateDialogMessage(WPARAM wParam, LPARAM lParam)
	{
		using namespace tools;

		// Retrieve major/minor version from .exe
		Path app_path{ Path::get_module_path() };
		std::string version{ get_file_ver(app_path.to_string()) };

		// Prepare the about text.
		std::string about_version{ tools::string_format(
			"FortiRDP %s (%s)\n",
			version.c_str(),
			get_plaform().c_str()) };
		about_version.append("Developed by Jean-Noel Meurisse");

		set_control_font(IDC_ABOUT_VERSION, _hFont);
		set_control_text(IDC_ABOUT_VERSION, tools::str2wstr(about_version));

		char mbedtls_ver[sizeof(MBEDTLS_VERSION_STRING_FULL)]{ 0 };
		mbedtls_version_get_string_full(mbedtls_ver);

		std::string about_info;
		about_info.append("A Fortigate SSLVPN client.\n");
		about_info.append("This program uses ");
		about_info.append(mbedtls_ver);
		about_info.append(" and lwIP ");
		about_info.append(LWIP_VERSION_STRING);
		about_info.append(" libraries.");

		set_control_font(IDC_ABOUT_INFO, _hFont);
		set_control_text(IDC_ABOUT_INFO, tools::str2wstr(about_info));

		center_window();

		if (::GetDlgCtrlID((HWND)wParam) != IDOK) {
			set_focus(IDOK);
			return FALSE;
		}

		return TRUE;
	}

	INT_PTR AboutDialog::onButtonClick(int cid, LPARAM lParam)
	{
		INT_PTR rc = FALSE;

		switch (cid) {
		case IDOK:
			close_dialog(TRUE);
			break;

		case IDCANCEL:
			close_dialog(FALSE);
			break;

		default:
			rc = TRUE;
			break;
		}

		return rc;
	}

}
