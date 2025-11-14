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
			// Get the next contiguous block of data.
			PBufQueue::cblock data_cblock{ get_cblock() };

			// Send this block.
			snd_status = socket.send_data(data_cblock.pdata, data_cblock.len);

			if (snd_status.code == NETCTX_SND_OK) {
				// Move the pointer into the queue if bytes have been sent.
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
			// Determine the space available in the TCP send buffer.
			const size_t send_buffer_size = tcp_sndbuf(socket);

			// Stop sending data if no more space available.
			if (send_buffer_size == 0)
				break;

			// Get a block of data but not larger than the available space
			// in the TCP send buffer.
			const PBufQueue::cblock data_cblock{ get_cblock(send_buffer_size) };

			// Compute the write flags.
			//  TCP_WRITE_FLAG_COPY is used to for lwIP to take a copy of the data
			//  TCP_WRITE_FLAG_MORE is set only if there are more data following this
			//  contiguous block.
			const u8_t flags = TCP_WRITE_FLAG_COPY | (data_cblock.more ? TCP_WRITE_FLAG_MORE : 0);

			// Send
			rc = tcp_write(socket, data_cblock.pdata, data_cblock.len, flags);
			if (rc)
				goto write_error;

			// Report the number of sent bytes.
			written += data_cblock.len;

			// Move the pointer into the queue if bytes have been sent
			if (!move(data_cblock.len)) {
				_logger->error("INTERNAL ERROR: OutputQueue::move failed");
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
