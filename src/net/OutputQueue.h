/*!
* This file is part of FortiRDP
*
* Copyright (C) 2025 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <lwip/tcp.h>
#include "net/Socket.h"
#include "tools/PBufQueue.h"
#include "tools/ErrUtil.h"
#include "tools/Logger.h"


namespace net {

	using namespace tools;

	class OutputQueue final : public PBufQueue
	{
	public:
		explicit OutputQueue(uint16_t capacity);
		~OutputQueue();

		net::snd_status write(Socket& socket);
		lwip_err write(struct tcp_pcb* socket, size_t& written);

	private:
		// The class name
		static const char* __class__;

		// a reference to the application logger
		Logger* const _logger;
	};

}
