/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <lwip/tcp.h>
#include "tools/PBufQueue.h"
#include "tools/ErrUtil.h"
#include "tools/Logger.h"


namespace net {

	using namespace tools;

	class LwipOutputQueue final : public PBufQueue
	{
	public:
		explicit LwipOutputQueue(uint16_t capacity);
		~LwipOutputQueue();

		lwip_err write(::tcp_pcb* socket, size_t& written);

	private:
		// a reference to the application logger
		Logger* const _logger;
	};

}
