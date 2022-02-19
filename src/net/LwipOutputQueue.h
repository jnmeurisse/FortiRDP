/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "tools/OutputQueue.h"
#include "tools/PBufChain.h"
#include "tools/ErrUtil.h"
#include "tools/Logger.h"

#include "lwip/tcp.h"

namespace net {
	using namespace tools;

	class LwipOutputQueue : public OutputQueue 
	{
	public:
		explicit LwipOutputQueue(int capacity);
		~LwipOutputQueue();


		lwip_err write(::tcp_pcb* socket, int& written);

	private:
		// a reference to the application logger
		Logger* const _logger;
	};
}