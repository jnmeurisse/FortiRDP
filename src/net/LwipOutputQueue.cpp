/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "LwipOutputQueue.h"

#include <algorithm>
#include <tools/PBufChain.h>
#include <cstdint>
#include <lwip/arch.h>


namespace net {

	using namespace tools;

	LwipOutputQueue::LwipOutputQueue(int capacity):
		OutputQueue(capacity),
		_logger(Logger::get_logger())
	{
		DEBUG_CTOR(_logger, "LwipOutputQueue");
	}


	LwipOutputQueue::~LwipOutputQueue()
	{
		DEBUG_DTOR(_logger, "LwipOutputQueue");
	}


	lwip_err LwipOutputQueue::write(::tcp_pcb* socket, size_t& written)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x enter LwipOutputQueue::write tcp=%x",
				(uintptr_t)this,
				(uintptr_t)socket);
		int rc = 0;
		written = 0;

		while (!empty()) {
			PBufChain* const pbuf = front();

			// Compute the length of the next chunk of data. The length of
			// a chunk is always less than 64 Kb, we can cast to an unsigned 16 Bits
			// integer.
			uint16_t available = static_cast<u16_t>(pbuf->cend() - pbuf->cbegin());

			// Determine how many bytes we can effectively send
			uint16_t len = min(tcp_sndbuf(socket), available);

			// stop writing if no more space available
			if (len == 0)
				break;

			// is this pbuf exhausted ?
			u8_t flags = TCP_WRITE_FLAG_COPY | ((available > len) ? TCP_WRITE_FLAG_MORE : 0);

			// send
			rc = tcp_write(socket, pbuf->cbegin(), len, flags);
			if (rc) 
				goto write_error;

			// report the number of sent bytes.
			written += len;

			// move our pointer into the payload if bytes have been sent
			pbuf->move(len);

			// unlink the first chain if no more data
			if (pbuf->empty()) {
				pop_front();
				delete pbuf;
			}
		}

		if (written > 0) {
			rc = tcp_output(socket);
			if (rc)
				goto write_error;
		}

	write_error:
		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x leave LwipOutputQueue::write tcp=%x rc=%d written=%d",
				(uintptr_t)this,
				(uintptr_t)socket,
				rc,
				written);

		if (rc == ERR_IF)
			// The output buffer was full, we will try to send the pbuf chain later
			rc = ERR_OK;

		return rc;
	}

}
