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


	netctx_snd_status WinsOutputQueue::write(Socket& socket)
	{
		netctx_snd_status snd_status = { NETCTX_SND_OK, 0, 0 };

		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x enter WinsOutputQueue::write tcp=%x",
				std::addressof(this),
				socket);

		if (!empty()) {
			PBufChain* const pbuf = front();

			// send what we can 
			snd_status = socket.send(pbuf->cbegin(), pbuf->cend() - pbuf->cbegin());

			if (snd_status.status_code == NETCTX_SND_OK) {
				// move our pointer into the payload if bytes have been sent
				pbuf->move(snd_status.sbytes);

				// unlink the first chain if no more data
				if (pbuf->empty()) {
					pop_front();
					delete pbuf;
				}
			}
		}

		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x leave WinsOutputQueue::write tcp=%x rc.errnum=%d rc.sbytes=%d",
				std::addressof(this),
				socket,
				snd_status.errnum, 
				snd_status.sbytes);


		// return the send status
		return snd_status;
	}
}