/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <iostream>

#include "tools/Logger.h"
#include "net/Socket.h"

namespace net {
	using namespace tools;

	Socket::Socket(mbedtls_net_context* netctx) :
		_logger(Logger::get_logger()),
		_netctx(netctx, &netctx_free)
	{
		DEBUG_CTOR(_logger, "Socket");
	}


	Socket::~Socket()
	{
		DEBUG_DTOR(_logger, "Socket");
	}


	bool Socket::is_connected() const
	{
		return get_fd() != -1;
	}

}
