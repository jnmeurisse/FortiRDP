/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "AsyncMessage.h"


AsyncMessage::AsyncMessage(UINT eventNumber):
	_id(eventNumber)
{
}


AsyncMessage::~AsyncMessage()
{
}


LRESULT AsyncMessage::send(HWND hWnd, void* lParam) const
{
	return ::SendMessage(hWnd, AsyncMessage::_registrationId, _id, (LPARAM) lParam);
}


BOOL AsyncMessage::post(HWND hWnd, void* lParam) const
{
	return ::PostMessage(hWnd, AsyncMessage::_registrationId, _id, (LPARAM)lParam);
}


UINT AsyncMessage::_registrationId = ::RegisterWindowMessage(L"fortirdp$message");


AsyncMessage AsyncMessage::OutputInfoMessageRequest(1);
AsyncMessage AsyncMessage::ShowErrorMessageDialogRequest(2);
AsyncMessage AsyncMessage::ShowInvalidCertificateDialogRequest(3);
AsyncMessage AsyncMessage::ShowAskCredentialDialogRequest(4);
AsyncMessage AsyncMessage::ShowAskCodeDialogRequest(5);
AsyncMessage AsyncMessage::DisconnectFromFirewallRequest(6);

AsyncMessage AsyncMessage::ConnectedEvent(100);
AsyncMessage AsyncMessage::DisconnectedEvent(101);
AsyncMessage AsyncMessage::TunnelListeningEvent(102);
