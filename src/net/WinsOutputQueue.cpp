/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "net/WinsOutputQueue.h"

namespace net {
	WinsOutputQueue::WinsOutputQueue(int capacity):
		OutputQueue(capacity),
		_logger(Logger::get_logger())
	{
		DEBUG_CTOR(_logger, "WinsOutputQueue");
	}


	WinsOutputQueue::~WinsOutputQueue()
	{
		DEBUG_DTOR(_logger, "WinsOutputQueue");
	}


	bool WinsOutputQueue::write(Socket& socket, size_t& written)
	{
		bool rs = false;
		written = 0;

		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x enter WinsOutputQueue::write tcp=%x",
				this,
				socket);

		if (!empty()) {
			PBufChain* const pbuf = front();

			// send what we can 
			switch (socket.send(pbuf->cbegin(), pbuf->cend() - pbuf->cbegin(), written))
			{
			case Socket::snd_ok: {
				// Move our pointer into the payload. Since 'written' is always lower 
				// than the maximum size of a single pbuf struct (64 Kbytes), we can 
				// safely cast to an int.
				pbuf->move(static_cast<int>(written));

				// unlink the first chain if no more data
				if (pbuf->empty()) {
					pop_front();
					delete pbuf;
				}

				rs = true;
			}
			break;

			case Socket::snd_retry: {
				rs = true;
			}
			break;

			case Socket::snd_error: {
				rs = false;
			}
			break;
			}
		}

		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x leave WinsOutputQueue::write tcp=%x rs=%d written=%d",
				this,
				socket,
				rs, 
				written);


		// return the status
		return rs;
	}
}