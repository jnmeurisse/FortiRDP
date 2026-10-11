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
		explicit OutputQueue(uint16_t capacity) noexcept;
		~OutputQueue();

		utl::mbed_err write(net::Socket& socket, size_t& written) noexcept;
		utl::lwip_err write(struct ::tcp_pcb* socket, size_t& written) noexcept;

	private:
		// The class name
		static const char* __class__;

		// a reference to the application logger
		utl::Logger& _logger;
	};

}
