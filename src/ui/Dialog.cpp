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

	INT_PTR CALLBACK MainDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == WM_INITDIALOG)
			::SetWindowLongPtr(hWnd, DWLP_USER, lParam);

		Dialog* const dialog = reinterpret_cast<Dialog*>(GetWindowLongPtr(hWnd, DWLP_USER));
		return dialog ? dialog->dialogProc(hWnd, message, wParam, lParam) : 0;
	}


	Dialog::Dialog(HINSTANCE hInstance, HWND hParent, int dialog_id) :
		_hInstance(hInstance),
		_hParent(hParent),
		_dialog_id(dialog_id),
		_hWindow(NULL_HWND)
	{
	}


	Dialog::~Dialog()
	{
	}


	HWND Dialog::control_handle(int control_id) const
	{
		return ::GetDlgItem(window_handle(), control_id);
	}


	bool Dialog::center_window(HWND hRelWindow) const
	{
		RECT rRelWindow;
		RECT rThisWindow;
		RECT r;

		if (hRelWindow == NULL_HWND) {
			hRelWindow = ::GetDesktopWindow();
		}

		if (!::GetWindowRect(hRelWindow, &rRelWindow))
			return false;
		if (!::GetWindowRect(window_handle(), &rThisWindow))
			return false;

		::CopyRect(&r, &rRelWindow);

		::OffsetRect(&rThisWindow, -rThisWindow.left, -rThisWindow.top);
		::OffsetRect(&r, -r.left, -r.top);
		::OffsetRect(&r, -rThisWindow.right, -rThisWindow.bottom);

		return ::SetWindowPos(
			window_handle(),
			HWND_TOP,
			rRelWindow.left + (r.right / 2), rRelWindow.top + (r.bottom / 2),
			0, 0,
			SWP_NOSIZE) != 0;
	}


	bool Dialog::show_window(int cmd_show) const
	{
		return ::ShowWindow(window_handle(), cmd_show) != 0;
	}


	bool Dialog::is_minimized() const
	{
		return ::IsIconic(window_handle()) != 0;
	}


	std::wstring Dialog::get_title() const
	{
		return Dialog::get_window_text(window_handle());
	}


	bool Dialog::set_title(const std::wstring& title) const
	{
		return Dialog::set_window_text(window_handle(), title);
	}


	bool Dialog::set_control_text(int control_id, const std::wstring& text) const
	{
		return Dialog::set_window_text(control_handle(control_id), text);
	}


	std::wstring Dialog::get_control_text(int control_id) const
	{
		return Dialog::get_window_text(control_handle(control_id));
	}


	void Dialog::set_control_textlen(int control_id, int length) const
	{
		::SendMessage(control_handle(control_id), EM_SETLIMITTEXT, length, 0);
	}


	void Dialog::set_control_enable(int control_id, bool enable) const
	{
		::EnableWindow(control_handle(control_id), enable);
	}


	bool Dialog::is_control_enabled(int control_id) const
	{
		return ::IsWindowEnabled(control_handle(control_id)) == TRUE;
	}


	void Dialog::set_control_visible(int control_id, bool visible) const
	{
		::ShowWindow(control_handle(control_id), visible ? SW_SHOW : SW_HIDE);
	}


	void Dialog::set_control_font(int control_id, HFONT font) const
	{
		::SendMessage(control_handle(control_id), WM_SETFONT, (WPARAM)font, TRUE);
	}


	std::wstring Dialog::get_window_text(HWND hWnd)
	{
		// Allocate a temporary buffer
		const size_t len = ::GetWindowTextLength(hWnd);
		std::vector<wchar_t> buffer(len + 1);

		// Get the window text
		::GetWindowText(hWnd, buffer.data(), len + 1);

		// Convert the buffer to a wstring
		return std::wstring(buffer.data(), len);
	}


	bool Dialog::set_window_text(HWND hWnd, const std::wstring& text)
	{
		return ::SetWindowText(hWnd, text.c_str()) != 0;
	}


	bool Dialog::set_focus(int control_id) const
	{
		return ::SetFocus(control_handle(control_id)) != NULL_HWND;
	}


	void Dialog::set_checkbox_state(int control_id, int state) const
	{
		::SendMessage(control_handle(control_id), BM_SETCHECK, state, 0);
	}


	bool Dialog::get_checkbox_state(int control_id) const
	{
		return ::SendMessage(control_handle(control_id), BM_GETCHECK, 0, 0) == BST_CHECKED;
	}


	bool Dialog::add_combo_text(int control_id, const std::wstring& text) const
	{
		return ::SendMessage(control_handle(control_id), CB_ADDSTRING, 0, (LPARAM)text.c_str()) >= 0;
	}


	bool Dialog::set_combo_index(int control_id, int index) const
	{
		return ::SendMessage(control_handle(control_id), CB_SETCURSEL, index, 0) == index;
	}


	int Dialog::get_combo_index(int control_id) const
	{
		return static_cast<int>(::SendMessage(control_handle(control_id), CB_GETCURSEL, 0, 0));
	}


	RECT Dialog::get_control_rect(int control_id) const
	{
		RECT bounds;

		::GetClientRect(control_handle(control_id), &bounds);
		return bounds;
	}


	RECT Dialog::get_client_rect() const
	{
		RECT bounds;

		::GetClientRect(window_handle(), &bounds);
		return bounds;
	}


	HMENU Dialog::get_sys_menu(bool reset) const
	{
		return ::GetSystemMenu(window_handle(), reset);
	}


	int Dialog::show_message_box(const std::wstring& message, UINT type) const
	{
		return ::MessageBox(window_handle(), message.c_str(), get_title().c_str(), type);
	}


	HWND Dialog::create_modeless_dialog()
	{
		return ::CreateDialogParam(
			_hInstance,
			MAKEINTRESOURCE(_dialog_id),
			_hParent,
			MainDialogProc,
			(LPARAM)this);
	}


	INT_PTR Dialog::create_modal_dialog()
	{
		return ::DialogBoxParam(
			_hInstance,
			MAKEINTRESOURCE(_dialog_id),
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


	INT_PTR Dialog::onCreateDialogMessage(WPARAM wParam, LPARAM lParam) 
	{
		WPARAM_UNUSED();
		LPARAM_UNUSED();
		return TRUE;
	}


	INT_PTR Dialog::onDestroyDialogMessage(WPARAM wParam, LPARAM lParam)
	{
		WPARAM_UNUSED();
		LPARAM_UNUSED();
		return FALSE;
	}


	INT_PTR Dialog::onCloseDialogMessage(WPARAM wParam, LPARAM lParam)
	{
		WPARAM_UNUSED();
		LPARAM_UNUSED();
		return FALSE;
	}


	INT_PTR Dialog::onCommandMessage(WPARAM wParam, LPARAM lParam)
	{
		LPARAM_UNUSED();
		int control_id = LOWORD(wParam);

		switch (HIWORD(wParam)) {
		case BN_CLICKED:		// Button or menu clicked
			return onButtonClick(control_id, lParam);

		case EN_CHANGE:			// Text changed
			return onTextChange(control_id, lParam);

		default:				// message not processed by this application
			return TRUE;
		}
	}


	INT_PTR Dialog::onSysCommandMessage(WPARAM wParam, LPARAM lParam)
	{
		WPARAM_UNUSED();
		LPARAM_UNUSED();
		return FALSE;
	}


	INT_PTR Dialog::onCtlColorStaticMessage(WPARAM wParam, LPARAM lParam)
	{
		WPARAM_UNUSED();
		LPARAM_UNUSED();
		return FALSE;
	}


	INT_PTR Dialog::onTimerMessage(WPARAM wParam, LPARAM lParam)
	{
		WPARAM_UNUSED();
		LPARAM_UNUSED();
		return FALSE;
	}


	INT_PTR Dialog::onHotKey(WPARAM wParam, LPARAM lParam)
	{
		WPARAM_UNUSED();
		LPARAM_UNUSED();
		return FALSE;
	}


	INT_PTR Dialog::onButtonClick(int control_id, LPARAM lParam) {
		CTRLID_UNUSED();
		LPARAM_UNUSED();
		return FALSE;
	}


	INT_PTR Dialog::onTextChange(int control_id, LPARAM lParam)
	{
		CTRLID_UNUSED();
		LPARAM_UNUSED();
		return FALSE;
	}


	INT_PTR Dialog::onAsyncMessage(UINT eventId, void* param)
	{
		UNREFERENCED_PARAMETER(eventId);
		UNREFERENCED_PARAMETER(param);

		return FALSE;
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
				rc = onAsyncMessage(static_cast<UINT>(wParam), reinterpret_cast<void*>(lParam));
			}
			else {
				// message was not processed
				rc = FALSE;
			}
		}

		return rc;
	}

}
