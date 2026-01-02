/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <Windows.h>
#include <memory>
#include "tools/Logger.h"


namespace ui {

	/**
	* Defines all windows user messages sent to the main window by the AsyncController.
	* AsyncMessages are sent from the AsyncController.  Two type of messages are defined :
	*  - request messages are sent using SendMessage API 
	*  - event messages are sent using PostMessage API.
	*
	* As the AsyncController and the main window are running into two different threads,
	* the system has to switch from the ActiveController thread to the main thread and
	* calls the appropriate window procedure to respond to the SendMessage.  Sent
	* messages are processed only when the receiving thread executes message retrieval code.
	* The main thread is not preempted (not interrupted) to execute the message procedure.
	* As a deadlock could occurs if SendMessage is sent while processing another message, we
	* use PostMessage which are executed asynchronously by the main loop.
	*/
	class AsyncMessage
	{
		using AsyncMessagePtr = std::unique_ptr<AsyncMessage>;

	public:
		virtual ~AsyncMessage();

		virtual LRESULT send_message(HWND hWnd, void* lParam) const = 0;
		virtual LRESULT send_message(HWND hWnd, const void* lParam) const;

		/**
		 * Checks if the message id correspond to the received registration id.
		*/
		static inline bool isAsyncMessage(UINT messageId) { return messageId == _windowsMessageId; }

		/**
		 * Returns this message id
		*/
		inline UINT id() const { return _id; }

		/*******************************************/
		/**** Requests sent to the main window  ****/

		/* request to display an error message box
		*/
		static AsyncMessagePtr ShowErrorMessageDialogRequest;

		/* request to display an Invalid Certificate error message box.
		*/
		static AsyncMessagePtr ShowInvalidCertificateDialogRequest;

		/* request to display a dialog that ask for credentials.
		*/
		static AsyncMessagePtr ShowCredentialsDialogRequest;

		/* request to display a dialog that ask for an additional code (pin code
		 * for example).
		*/
		static AsyncMessagePtr ShowPinCodeDialogRequest;

		/* request to display a dialog to authenticate using SAML.
		*/
		static AsyncMessagePtr ShowSamlAuthDialogRequest;

		/* request to execute the disconnection procedure.
		*/
		static AsyncMessagePtr DisconnectFromFirewallRequest;

		/*******************************************/
		/*** Events sent to the main window.     ***/

		/* informs that the client portal is connected (or fail to connect)
		 * to the firewall.
		*/
		static AsyncMessagePtr ConnectedEvent;

		/* informs that the client portal is disconnected from the firewall.
		*/
		static AsyncMessagePtr DisconnectedEvent;

		/* informs that the tunnel is connected to the firewall and listening for
		 * local incoming communication.
		*/
		static AsyncMessagePtr TunnelListeningEvent;

		/* request to display a message string in the output text box.
		*/
		static AsyncMessagePtr OutputInfoEvent;

	protected:
		explicit AsyncMessage(UINT eventNumber);

		// The application logger.
		aux::Logger* const _logger;

		// Global message identifier assigned by windows for all AsyncMessages.
		static UINT _windowsMessageId;

		// The AsyncMessage id.
		const UINT _id;
	};

}
