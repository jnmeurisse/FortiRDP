/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "AsyncMessage.h"

namespace ui {

	AsyncMessage::AsyncMessage(UINT eventNumber) :
		_logger(utl::Logger::get_logger()),
		_id(eventNumber)
	{
	}


	AsyncMessage::~AsyncMessage()
	{
	}


	LRESULT AsyncMessage::send_message(HWND hWnd, const void* lParam) const
	{
		return send_message(hWnd, const_cast<void*>(lParam));
	}


	UINT AsyncMessage::_windowsMessageId = ::RegisterWindowMessage(L"fortirdp$message");


	class AsyncRequestMessage : public AsyncMessage
	{
	public:
		explicit AsyncRequestMessage(UINT eventNumber) :
			AsyncMessage(eventNumber)
		{
		}


		~AsyncRequestMessage() override
		{
		}


		LRESULT send_message(HWND hWnd, void* lParam) const override
		{
			return ::SendMessage(hWnd, AsyncMessage::_windowsMessageId, _id, (LPARAM)lParam);
		}

	private:
		// The class name.
		static const char* __class__;
	};

	const char* AsyncRequestMessage::__class__ = "AsyncRequestMessage";


	class AsyncEventMessage : public AsyncMessage
	{
	public:
		explicit AsyncEventMessage(UINT eventNumber) :
			AsyncMessage(eventNumber)
		{
		}


		~AsyncEventMessage() override
		{
		}


		LRESULT send_message(HWND hWnd, void* lParam) const override
		{
			return ::PostMessage(hWnd, AsyncMessage::_windowsMessageId, _id, (LPARAM)lParam);
		}


	private:
		// The class name.
		static const char* __class__;
	};


	const char* AsyncEventMessage::__class__ = "AsyncEventMessage";


	std::unique_ptr<AsyncMessage> AsyncMessage::ShowErrorMessageDialogRequest(new AsyncRequestMessage(1));
	std::unique_ptr<AsyncMessage> AsyncMessage::ShowInvalidCertificateDialogRequest(new AsyncRequestMessage(2));
	std::unique_ptr<AsyncMessage> AsyncMessage::ShowCredentialsDialogRequest(new AsyncRequestMessage(3));
	std::unique_ptr<AsyncMessage> AsyncMessage::ShowPinCodeDialogRequest(new AsyncRequestMessage(4));
	std::unique_ptr<AsyncMessage> AsyncMessage::ShowSamlAuthDialogRequest(new AsyncRequestMessage(5));
	std::unique_ptr<AsyncMessage> AsyncMessage::DisconnectFromFirewallRequest(new AsyncRequestMessage(6));

	std::unique_ptr<AsyncMessage> AsyncMessage::ConnectedEvent(new AsyncEventMessage(10));
	std::unique_ptr<AsyncMessage> AsyncMessage::DisconnectedEvent(new AsyncEventMessage(11));
	std::unique_ptr<AsyncMessage> AsyncMessage::TunnelListeningEvent(new AsyncEventMessage(12));
	std::unique_ptr<AsyncMessage> AsyncMessage::OutputInfoEvent(new AsyncEventMessage(13));

}
