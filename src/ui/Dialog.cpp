/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Dialog.h"

#include <vector>
#include "ui/AsyncMessage.h"

namespace ui {

	static INT_PTR CALLBACK MainDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == WM_INITDIALOG)
			::SetWindowLongPtr(hWnd, DWLP_USER, lParam);

		Dialog* const dialog = (Dialog*)GetWindowLongPtr(hWnd, DWLP_USER);
		return dialog ? dialog->dialogProc(hWnd, message, wParam, lParam) : 0;
	}


	Dialog::Dialog(HINSTANCE hInstance, HWND hParent, int idd) :
		_hWindow(0),
		_hInstance(hInstance),
		_hParent(hParent),
		_idd(idd)
	{
	}


	Dialog::~Dialog()
	{
	}


	HWND Dialog::control_handle(int idc) const
	{
		return ::GetDlgItem(window_handle(), idc);
	}


	bool Dialog::center_window(HWND hRelWindow)
	{
		RECT rRelWindow;
		RECT rThisWindow;
		RECT r;

		if (hRelWindow == NULL) {
			// center
			hRelWindow = ::GetDesktopWindow();
		}

		if (!::GetWindowRect(hRelWindow, &rRelWindow))
			goto error;
		if (!::GetWindowRect(window_handle(), &rThisWindow))
			goto error;

		::CopyRect(&r, &rRelWindow);

		::OffsetRect(&rThisWindow, -rThisWindow.left, -rThisWindow.top);
		::OffsetRect(&r, -r.left, -r.top);
		::OffsetRect(&r, -rThisWindow.right, -rThisWindow.bottom);

		if (!::SetWindowPos(window_handle(),
			HWND_TOP,
			rRelWindow.left + (r.right / 2), rRelWindow.top + (r.bottom / 2),
			0, 0,
			SWP_NOSIZE))
			goto error;

		return true;

	error:
		return false;
	}


	bool Dialog::show_window(int cmd_show)
	{
		return ::ShowWindow(window_handle(), cmd_show) == TRUE;
	}


	bool Dialog::is_minimized()
	{
		return ::IsIconic(window_handle()) == TRUE;
	}


	std::wstring Dialog::get_title() const
	{
		return Dialog::get_window_text(window_handle());
	}


	bool Dialog::set_title(const std::wstring& title)
	{
		return Dialog::set_window_text(window_handle(), title);
	}


	bool Dialog::set_control_text(int idc, const std::wstring& text)
	{
		return Dialog::set_window_text(control_handle(idc), text);
	}


	std::wstring Dialog::get_control_text(int idc) const
	{
		return Dialog::get_window_text(control_handle(idc));
	}


	void Dialog::set_control_textlen(int idc, int length)
	{
		::SendMessage(control_handle(idc), EM_SETLIMITTEXT, length, 0);
	}


	void Dialog::set_control_enable(int idc, bool enable)
	{
		::EnableWindow(control_handle(idc), enable);
	}


	bool Dialog::is_control_enabled(int idc) const
	{
		return ::IsWindowEnabled(control_handle(idc)) == TRUE;
	}


	void Dialog::set_control_visible(int idc, bool visible)
	{
		::ShowWindow(control_handle(idc), visible ? SW_SHOW : SW_HIDE);
	}


	void Dialog::set_control_font(int idc, HFONT font)
	{
		::SendMessage(control_handle(idc), WM_SETFONT, (WPARAM)font, TRUE);
	}


	std::wstring Dialog::get_window_text(HWND hWnd)
	{
		// allocate a temporary buffer
		int len = ::GetWindowTextLength(hWnd);
		std::vector<wchar_t> buffer(len + 1);

		// get the window text
		::GetWindowText(hWnd, &buffer[0], len + 1);

		// convert the buffer as a wstring
		return std::wstring(&buffer[0], len);
	}


	bool Dialog::set_window_text(HWND hWnd, const std::wstring& text)
	{
		return ::SetWindowText(hWnd, text.c_str()) != 0;
	}


	bool Dialog::set_focus(int idc)
	{
		return ::SetFocus(control_handle(idc)) != NULL;
	}


	void Dialog::set_checkbox_state(int idc, int state)
	{
		::SendMessage(control_handle(idc), BM_SETCHECK, state, 0);
	}


	bool Dialog::get_checkbox_state(int idc) const
	{
		return ::SendMessage(control_handle(idc), BM_GETCHECK, 0, 0) == BST_CHECKED;
	}


	bool Dialog::add_combo_text(int idc, const std::wstring& text)
	{
		return ::SendMessage(control_handle(idc), CB_ADDSTRING, 0, (LPARAM)text.c_str()) >= 0;
	}


	bool Dialog::set_combo_index(int idc, int index)
	{
		return ::SendMessage(control_handle(idc), CB_SETCURSEL, index, 0) == index;
	}


	int Dialog::get_combo_index(int idc) const
	{
		return static_cast<int>(::SendMessage(control_handle(idc), CB_GETCURSEL, 0, 0));
	}


	RECT Dialog::get_control_rect(int idc) const
	{
		RECT bounds;

		::GetClientRect(control_handle(idc), &bounds);
		return bounds;
	}


	RECT Dialog::get_client_rect() const
	{
		RECT bounds;

		::GetClientRect(window_handle(), &bounds);
		return bounds;
	}


	HMENU Dialog::get_sys_menu(bool reset)
	{
		return ::GetSystemMenu(window_handle(), reset);
	}


	int Dialog::show_message_box(const std::wstring& message, UINT type)
	{
		return ::MessageBox(window_handle(), message.c_str(), get_title().c_str(), type);
	}


	HWND Dialog::create_modeless_dialog()
	{
		return ::CreateDialogParam(
			_hInstance,
			MAKEINTRESOURCE(_idd),
			_hParent,
			MainDialogProc,
			(LPARAM)this);
	}


	INT_PTR Dialog::create_modal_dialog()
	{
		return ::DialogBoxParam(
			_hInstance,
			MAKEINTRESOURCE(_idd),
			_hParent,
			MainDialogProc,
			(LPARAM)this);
	}

	HFONT Dialog::create_font(int size, const std::wstring& name)
	{
		HDC hdc = ::GetDC(_hWindow);
		int logsize = -::MulDiv(size, GetDeviceCaps(hdc, LOGPIXELSY), 72);

		return ::CreateFont(
			logsize,
			0,
			0,
			0,
			FW_DONTCARE,
			FALSE,
			FALSE,
			FALSE,
			DEFAULT_CHARSET,
			OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY,
			VARIABLE_PITCH,
			name.c_str());
	}

	INT_PTR Dialog::onCommandMessage(WPARAM wParam, LPARAM lParam)
	{
		int cid = LOWORD(wParam);

		switch (HIWORD(wParam)) {
		case BN_CLICKED:		// Button or menu clicked
			return onButtonClick(cid, lParam);
			break;

		default:
			// message not processed by this application
			return TRUE;
		}
	}


	INT_PTR Dialog::dialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		INT_PTR rc;

		switch (message) {
		case WM_INITDIALOG:
			_hWindow = hWnd;
			rc = onCreateDialogMessage(wParam, lParam);
			break;

		case WM_DESTROY:
			rc = onDestroyDialogMessage(wParam, lParam);
			break;

		case WM_SYSCOMMAND:
			rc = onSysCommandMessage(wParam, lParam);
			break;

		case WM_COMMAND:
			rc = onCommandMessage(wParam, lParam);
			break;

		case WM_CTLCOLORSTATIC:
			rc = onCtlColorStaticMessage(wParam, lParam);
			break;

		case WM_TIMER:
			rc = onTimerMessage(wParam, lParam);
			break;

		case WM_HOTKEY:
			rc = onHotKey(wParam, lParam);
			break;

		default:
			if (AsyncMessage::isAsyncMessage(message)) {
				rc = onUserEventMessage((UINT)wParam, (void*)lParam);
			}
			else {
				// message was not processed
				rc = FALSE;
			}
		}

		return rc;
	}

}
