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

/*
* Defines all windows user messages sent to the main window by the AsyncController.
* Requests are sent with a SendMessage while Events are sent with a PostMessage.
*
* As the AsyncController and the main window are running into two different threads,
* the system has to switch from the ActiveController thread to the main thread and
* calls the appropriate window procedure to respond to the SendMessage.  Sent
* messages are processed only when the receiving thread executes message retrieval code.
* The main thread is not preempted (not interrupted) to execute the message procedure.
* As a deadlock could occurs if SendMessage is sent while processing another message, we
* use PostMessage which are executed asynchronously by the main loop.
*/
class AsyncMessage final
{
public:
	LRESULT send(HWND hWnd, void* lParam) const;
	BOOL post(HWND hWnd, void* lParam) const;

	/* checks if the message id correspond to the received registration id 
	*/
	static inline bool isAsyncMessage(UINT messageId) { return messageId == _registrationId; }

	/* checks if the specified id corresponds to this AsyncMessage
	*/
	inline bool operator ==(UINT id) const { return id == _id; }

	/***
	* Requests sent to the main window
	*/

	/* request to display a message string in the output text box
	*/
	static AsyncMessage OutputInfoMessageRequest;

	/* request to display an error message box
	*/
	static AsyncMessage ShowErrorMessageDialogRequest;
	
	/* request to display an Invalid Certificate error message box
	*/
	static AsyncMessage ShowInvalidCertificateDialogRequest;

	/* request to display a dialog that ask for credentials
	*/
	static AsyncMessage ShowAskCredentialDialogRequest;

	/* request to display a dialog that ask for an additional code (pin code for example)
	*/
	static AsyncMessage ShowAskCodeDialogRequest;

	/* execute the disconnection procedure
	*/
	static AsyncMessage DisconnectFromFirewallRequest;

	/***
	* Events sent to the main window.
	*/

	/* informs that the client portal is connected (or fail) to the firewall
	*/
	static AsyncMessage ConnectedEvent;

	/* informs that the client portal is disconnected from the firewall
	*/
	static AsyncMessage DisconnectedEvent;

	/* informs that the tunnel is connected to the firewall and listening for local incoming communication
	*/
	static AsyncMessage TunnelListeningEvent;


private:
	explicit AsyncMessage(UINT eventNumber);
	virtual ~AsyncMessage();

private:
	// - A global message identifier assigned by windows for all AsyncMessages
	static UINT _registrationId;

	// - This AsyncMessage id
	const UINT _id;
};


