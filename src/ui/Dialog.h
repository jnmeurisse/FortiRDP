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

#define WPARAM_UNUSED() UNREFERENCED_PARAMETER(wParam)
#define LPARAM_UNUSED() UNREFERENCED_PARAMETER(lParam)
#define CTRLID_UNUSED() UNREFERENCED_PARAMETER(control_id)


namespace ui {


	/**
	* A Dialog abstract base class.
	*/
	class Dialog
	{
	public:
		explicit Dialog(HINSTANCE hInstance, HWND hParent, int dialog_id);
		virtual ~Dialog();

		/**
		 * Returns the handle to the current instance of this application.
		*/
		inline HINSTANCE instance_handle() const { return _hInstance; }

		/**
		 * Returns the handle to this dialog window.
		 *
		 * This handle is available only after the dialog processed the 
		 * OnCreateDialogMessage windows event.
		*/
		inline HWND window_handle() const { return _hWindow; }

		/**
		 * Returns the handle of the given control.
		 */
		HWND control_handle(int control_id) const;

		/**
		 * Centers the dialog window relative to the given window.
		 * 
		 * If not specified the function centers the dialog window relative to the
		 * screen.
		*/
		bool center_window(HWND hRelWindow = NULL);

		/**
		 * Shows the window.
		*/
		bool show_window(int cmd_show);

		/**
		 * Returns true if this dialog window is minimized.
		*/
		bool is_minimized();

		/**
		 * Returns the dialog window title.
		*/
		std::wstring get_title() const;

		/**
		 * Sets the dialog window title.
		 * 
		 * @return true if the function succeeds.
		*/
		bool set_title(const std::wstring& title);

		/**
		 * Sets the text of a the given dialog control.
		 * 
 		 * @return true if the function succeeds.
		*/
		bool set_control_text(int control_id, const std::wstring& text);

		/**
		 * Returns the text from a specified dialog control.
		*/
		std::wstring get_control_text(int control_id) const;

		/**
		 * Sets the text length limit for the specified dialog control.
		*/
		void set_control_textlen(int control_id, int length);

		/**
		 * Enables or disables a dialog control.
		*/
		void set_control_enable(int control_id, bool enable);

		/**
		 * Checks if a dialog control is enabled.
		*/
		bool is_control_enabled(int control_id) const;

		/**
		 * Shows or hides a dialog control.
		*/
		void set_control_visible(int control_id, bool visible);

		/**
		 * Assigns a font to a dialog control.
		*/
		void set_control_font(int control_id, HFONT font);

		/**
		 * Sets the mouse focus to the specified control.
		*/
		bool set_focus(int control_id);

		/**
		 * Sets a check box state.
		*/
		void set_checkbox_state(int control_id, int state);

		/**
		 * Gets a check box state.
		 * 
		 * @return true of the box is checked.
		*/
		bool get_checkbox_state(int control_id) const;

		/**
		 * Adds a text to the specified combo box.
		 * 
		 * @return true if the function succeeds.
		*/
		bool add_combo_text(int control_id, const std::wstring& text);

		/**
		 * Selects a string at position 'index' in the combo box.
		 * 
		 * @return true if the function succeeds.
		*/
		bool set_combo_index(int control_id, int index);

		/**
		 * Returns the selected string in the combo box.
		*/
		int get_combo_index(int control_id) const;

		/**
		 * Returns the dialog control rectangle.
		*/
		RECT get_control_rect(int control_id) const;

		/**
		 * Returns the dialog window client rectangle.
		*/
		RECT get_client_rect() const;

		/**
		 * Returns the system menu handle.
		*/
		HMENU get_sys_menu(bool reset);

		/**
		 * Shows a message box.
		*/
		int show_message_box(const std::wstring& message, UINT type);

	protected:
		/**
		* Creates a modeless dialog.
		* 
		* @return a window handle to the dialog box.
		*/
		HWND create_modeless_dialog();

		/**
		* Creates a modal dialog.
		* 
		* @return the value of the result parameter specified in the call to the 
		* EndDialog function used to terminate the dialog box.
		*/
		INT_PTR create_modal_dialog();

		/**
		* Create a font.
		*/
		HFONT create_font(int size, const std::wstring& name);

		// Windows Event handlers
		virtual INT_PTR onCreateDialogMessage(WPARAM wParam, LPARAM lParam);
		virtual INT_PTR onDestroyDialogMessage(WPARAM wParam, LPARAM lParam);
		virtual INT_PTR onCloseDialogMessage(WPARAM wParam, LPARAM lParam);
		virtual INT_PTR onCommandMessage(WPARAM wParam, LPARAM lParam);
		virtual INT_PTR onSysCommandMessage(WPARAM wParam, LPARAM lParam);
		virtual INT_PTR onCtlColorStaticMessage(WPARAM wParam, LPARAM lParam);
		virtual INT_PTR onTimerMessage(WPARAM wParam, LPARAM lParam);
		virtual INT_PTR onHotKey(WPARAM wParam, LPARAM lParam);
		virtual INT_PTR onButtonClick(int control_id, LPARAM lParam);
		virtual INT_PTR onTextChange(int control_id, LPARAM lParam);
		virtual INT_PTR onUserEventMessage(UINT eventNumber, void* param);

	private:
		// The application instance handle.
		const HINSTANCE _hInstance;

		// The parent window handle.
		const HWND _hParent;

		// This dialog id.
		const int _dialog_id;

		// This dialog handle.
		HWND _hWindow;

		/* The global window dialog procedure dispatches events to dialogProc.
		*/
		friend static INT_PTR CALLBACK MainDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		/* The dialog procedure dispatches events to virtual event handlers.
		*/
		INT_PTR dialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		// Helper functions to get and set a window text.
		static std::wstring get_window_text(HWND hWnd);
		static bool set_window_text(HWND hWnd, const std::wstring& text);
	};

}
