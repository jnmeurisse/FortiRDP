/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "WinsOutputQueue.h"

#include <memory>


namespace net {

	WinsOutputQueue::WinsOutputQueue(size_t capacity):
		OutputQueue(capacity),
		_logger(Logger::get_logger())
	{
		DEBUG_CTOR(_logger, "WinsOutputQueue");
	}


	WinsOutputQueue::~WinsOutputQueue()
	{
		DEBUG_DTOR(_logger, "WinsOutputQueue");
	}


	net::snd_status WinsOutputQueue::write(Socket& socket)
	{
		snd_status snd_status { NETCTX_SND_OK, 0, 0 };

		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x enter WinsOutputQueue::write tcp=%x",
				(uintptr_t)this,
				(uintptr_t)std::addressof(socket));

		int rc = 0;

		if (!empty()) {
			PBufChain* const pbuf = front();

			// send what we can 
			snd_status = socket.send_data(pbuf->cbegin(), pbuf->cend() - pbuf->cbegin());

			if (snd_status.code == NETCTX_SND_OK) {
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
				".... %x leave WinsOutputQueue::write tcp=%x rc=%d written=%d",
				(uintptr_t)this,
				(uintptr_t)std::addressof(socket),
				rc, 
				snd_status.sbytes);


		// return the status
		return snd_status;
	}

}
