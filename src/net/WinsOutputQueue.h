/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "net/Socket.h"
#include "tools/Logger.h"
#include "tools/PBufQueue.h"


namespace net {

	using namespace tools;

	class WinsOutputQueue final : public PBufQueue

	{
	public:
		explicit WinsOutputQueue(uint16_t capacity);
		~WinsOutputQueue();

		/* Writes next chunk of data available in this output queue.
		*
		*/
		net::snd_status write(Socket& socket);

	private:
		// a reference to the application logger
		Logger* const _logger;
	};

}
