/*!
* This file is part of FortiRDP
*
* Copyright (C) 2025 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "OutputQueue.h"

namespace net {

	OutputQueue::OutputQueue(uint16_t capacity) : 
		PBufQueue(capacity),
		_logger(Logger::get_logger())
	{
		DEBUG_CTOR(_logger, "OutputQueue");
	}


	OutputQueue::~OutputQueue()
	{
		DEBUG_DTOR(_logger, "OutputQueue");
	}


	net::snd_status OutputQueue::write(Socket& socket)
	{
		snd_status snd_status{ NETCTX_SND_OK, 0, 0 };

		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x enter OutputQueue::write mbedtls_socket=%x",
				(uintptr_t)this,
				(uintptr_t)std::addressof(socket));

		if (!is_empty()) {
			// Send what is available from the head pbuf in this output queue. 
			snd_status = socket.send_data(cbegin(), cend() - cbegin());

			if (snd_status.code == NETCTX_SND_OK) {
				// move into the payload if bytes have been sent.
				if (!move(snd_status.sbytes))
					_logger->error("INTERNAL ERROR: OutputQueue::move failed");
			}
		}

		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x leave OutputQueue::write mbedtls_socket=%x status=%d rc=%d written=%zu",
				(uintptr_t)this,
				(uintptr_t)std::addressof(socket),
				snd_status.code,
				snd_status.rc,
				snd_status.sbytes);


		// return the status
		return snd_status;
	}


	lwip_err OutputQueue::write(struct tcp_pcb* socket, size_t& written)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x enter OutputQueue::write lwip_socket=%x",
				(uintptr_t)this,
				(uintptr_t)socket);

		written = 0;

		int rc = 0;

		while (!is_empty()) {

			// Compute the length of the next chunk of data. The length of
			// a chunk is always less than 64 Kb, we can cast to an unsigned 16 Bits
			// integer.
			uint16_t available = static_cast<u16_t>(cend() - cbegin());

			// Determine how many bytes we can effectively send
			uint16_t len = min(tcp_sndbuf(socket), available);

			// stop writing if no more space available
			if (len == 0)
				break;

			// is this pbuf exhausted ?
			u8_t flags = TCP_WRITE_FLAG_COPY | ((available > len) ? TCP_WRITE_FLAG_MORE : 0);

			// send
			rc = tcp_write(socket, cbegin(), len, flags);
			if (rc)
				goto write_error;

			// report the number of sent bytes.
			written += len;

			// move our pointer into the payload if bytes have been sent
			move(len);
		}

		if (written > 0) {
			rc = tcp_output(socket);
			if (rc)
				goto write_error;
		}

	write_error:
		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x leave OutputQueue::write lwip_socket=%x rc=%d written=%d",
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
