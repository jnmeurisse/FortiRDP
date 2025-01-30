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

	WinsOutputQueue::WinsOutputQueue(uint16_t capacity):
		PBufQueue(capacity),
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

		if (!is_empty()) {
			// Send what is available from the head pbuf in this output queue. 
			snd_status = socket.send_data(cbegin(), cend() - cbegin());

			if (snd_status.code == NETCTX_SND_OK) {
				// move into the payload if bytes have been sent.
				if (!move(snd_status.sbytes))
					_logger->error("INTERNAL ERROR: WinsOutputQueue::move failed");
			}
		}

		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x leave WinsOutputQueue::write tcp=%x status=%d rc=%d written=%zu",
				(uintptr_t)this,
				(uintptr_t)std::addressof(socket),
				snd_status.code,
				snd_status.rc,
				snd_status.sbytes);


		// return the status
		return snd_status;
	}

}
