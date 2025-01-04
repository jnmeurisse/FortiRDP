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


	mbed_err WinsOutputQueue::write(Socket& socket, size_t& written)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x enter WinsOutputQueue::write tcp=%x",
				(uintptr_t)this,
				(uintptr_t)std::addressof(socket));

		int rc = 0;
		written = 0;

		if (!empty()) {
			PBufChain* const pbuf = front();

			// send what we can 
			rc = socket.send(pbuf->cbegin(), pbuf->cend() - pbuf->cbegin());

			if (rc > 0) {
				// reports the number of sent bytes.
				written = rc;

				// move our pointer into the payload if bytes have been sent
				pbuf->move(rc);

				// unlink the first chain if no more data
				if (pbuf->empty()) {
					pop_front();
					delete pbuf;
				}

				// no error detected 
				rc = 0;
			}
		}

		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x leave WinsOutputQueue::write tcp=%x rc=%d written=%d",
				(uintptr_t)this,
				(uintptr_t)std::addressof(socket),
				rc, 
				written);


		// return the error code
		return rc;
	}

}
