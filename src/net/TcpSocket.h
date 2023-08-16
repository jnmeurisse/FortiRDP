/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "net/Socket.h"


namespace net {

	class TcpSocket : public Socket {
	public:
		explicit TcpSocket();
		explicit TcpSocket(::BIO* bio);
		virtual ~TcpSocket();

		virtual bool set_nodelay(bool no_delay) override;

	protected:
		virtual bool do_connect(int timeout) override;
	};

}