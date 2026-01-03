/*!
* This file is part of FortiRDP
*
* Copyright (C) 2025 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <cstdint>
#include <lwip/tcp.h>
#include "net/Socket.h"
#include "util/PBufQueue.h"
#include "util/ErrUtil.h"
#include "util/Logger.h"


namespace net {

	class OutputQueue final : public utl::PBufQueue
	{
	public:
		explicit OutputQueue(uint16_t capacity);
		~OutputQueue();

		net::snd_status write(net::Socket& socket);
		utl::lwip_err write(struct ::tcp_pcb* socket, size_t& written);

	private:
		// The class name
		static const char* __class__;

		// a reference to the application logger
		utl::Logger* const _logger;
	};

}
