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

namespace ui {

	/*
	* A Dialog abstract base class
	*/
	class Dialog
	{
	public:
		explicit Dialog(HINSTANCE hInstance, HWND hParent, int idd);
		virtual ~Dialog();

		/* Returns the handle to the current instance of this application.
		*/
		inline HINSTANCE instance_handle() const { return _hInstance; }

		/* Returns the handle to this dialog window. This handle is available
		 * only after the dialog processed the OnCreateDialogMessage.
		*/
		inline HWND window_handle() const { return _hWindow; }

		/* Returns the handle of the given control
		 */
		HWND control_handle(int idc) const;

		/* Centers the dialog window relative to the given window. If not specified
		 * the function centers the dialog window relative to the screen.
		*/
		bool center_window(HWND hRelWindow = NULL);

		/* Shows the window.
		*/
		bool show_window(int cmd_show);

		/* Returns true if windows is minimized.
		*/
		bool is_minimized();

		/* Returns the dialog window title.
		*/
		std::wstring get_title() const;

		/* Set the dialog window title.
		*  @return true if the function succeeds.
		*/
		bool set_title(const std::wstring& title);

		/* Set the text of a the given control
		*  @return true if the function succeeds.
		*/
		bool set_control_text(int idc, const std::wstring& text);

		/* Returns text from a specified control.
		*/
		std::wstring get_control_text(int idc) const;

		/* Sets the text length limit for the specified control.
		*/
		void set_control_textlen(int idc, int length);

		/* Enables/disables a control.
		*/
		void set_control_enable(int idc, bool enable);

		/* Checks if a control is enabled or not.
		*/
		bool is_control_enabled(int idc) const;

		/* Shows/hides a control.
		*/
		void set_control_visible(int idc, bool visible);

		/* Sets control font.
		*/
		void set_control_font(int idc, HFONT font);

		/* Sets the mouse focus to the specified control.
		*/
		bool set_focus(int idc);

		/* Sets a check box state.
		*/
		void set_checkbox_state(int idc, int state);

		/* Gets a check box state.
		*/
		bool get_checkbox_state(int idc) const;

		/* Adds a text to the specified combo box.
		*  @return true if the function succeeds.
		*/
		bool add_combo_text(int idc, const std::wstring& text);

		/* Selects a string at position 'index' in the combobox.
		*  @return true if the function succeeds.
		*/
		bool set_combo_index(int idc, int index);

		/* Returns the selected string in the combobox.
		*/
		int get_combo_index(int idc) const;

		/* Returns the control rectangle.
		*/
		RECT get_control_rect(int idc) const;

		/* Returns the window client rectangle.
		*/
		RECT get_client_rect() const;

		/* Returns the system menu.
		*/
		HMENU get_sys_menu(bool reset);

		/* Shows a message box.
		*/
		int show_message_box(const std::wstring& message, UINT type);

	protected:
		// Methods to create a modal or a modeless dialog
		HWND create_modeless_dialog();
		INT_PTR create_modal_dialog();

		// Method to create a font
		HFONT create_font(int size, const std::wstring& name);

		// Event handlers
		virtual INT_PTR onCreateDialogMessage(WPARAM wParam, LPARAM lParam) { return TRUE; }
		virtual INT_PTR onDestroyDialogMessage(WPARAM wParam, LPARAM lParam) { return FALSE; }
		virtual INT_PTR onCloseDialogMessage(WPARAM wParam, LPARAM lParam) { return FALSE; }
		virtual INT_PTR onCommandMessage(WPARAM wParam, LPARAM lParam);
		virtual INT_PTR onSysCommandMessage(WPARAM wParam, LPARAM lParam) { return FALSE; }
		virtual INT_PTR onCtlColorStaticMessage(WPARAM wParam, LPARAM lParam) { return FALSE; }
		virtual INT_PTR onTimerMessage(WPARAM wParam, LPARAM lParam) { return FALSE; }
		virtual INT_PTR onHotKey(WPARAM wParam, LPARAM lParam) { return FALSE; }
		virtual INT_PTR onButtonClick(int idc, LPARAM lParam) { return FALSE; }
		virtual INT_PTR onUserEventMessage(UINT eventNumber, void* param) { return FALSE; }

	private:
		// The application instance handle
		const HINSTANCE _hInstance;

		// The parent window handle
		const HWND _hParent;

		// This dialog id
		const int _idd;

		// This dialog handle
		HWND _hWindow;

		/* the global window dialog procedure dispatches events to dialogProc
		*/
		friend static INT_PTR CALLBACK MainDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		/* the dialog procedure dispatches events to virtual event handlers
		*/
		INT_PTR dialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		// Helper functions to get and set the window text
		static std::wstring get_window_text(HWND hWnd);
		static bool set_window_text(HWND hWnd, const std::wstring& text);
	};

}
