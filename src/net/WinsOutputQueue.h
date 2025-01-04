/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <list>
#include "net/Socket.h"
#include "tools/PBufChain.h"
#include "tools/OutputQueue.h"
#include "tools/Logger.h"

namespace net {
	using namespace tools;

	class WinsOutputQueue final : public OutputQueue
	{
	public:
		explicit WinsOutputQueue(int capacity);
		~WinsOutputQueue();

		/* Writes next chunk of data available in this output queue.
		*/
		netctx_snd_status write(Socket& socket);

	private:
		// a reference to the application logger
		Logger* const _logger;
	};
}