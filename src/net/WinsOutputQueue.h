/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "net/Socket.h"
#include "tools/ErrUtil.h"
#include "tools/Logger.h"
#include "tools/OutputQueue.h"


namespace net {
	using namespace tools;

	class WinsOutputQueue final : public OutputQueue
	{
	public:
		explicit WinsOutputQueue(int capacity);
		~WinsOutputQueue();

		/* Writes next chunk of data available in this output queue.
		*
		* The function returns the number of bytes sent, which can be less
		* than the used space in the queue. If an error has occurred, the 
		* function returns a negative error code. 
		*/
		mbed_err write(Socket& socket, size_t& written);

	private:
		// a reference to the application logger
		Logger* const _logger;
	};
}