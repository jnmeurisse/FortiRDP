/*!
* This file is part of FortiRDP
*
* Copyright (C) 2025 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "OutputQueue.h"
#include <memory>

namespace net {
	using namespace utl;


	OutputQueue::OutputQueue(uint16_t capacity) :
		PBufQueue(capacity),
		_logger(Logger::get_logger())
	{
		DEBUG_CTOR(_logger);
	}


	OutputQueue::~OutputQueue()
	{
		DEBUG_DTOR(_logger);
	}


	utl::mbed_err OutputQueue::write(net::Socket& socket, size_t& written)
	{
		TRACE_ENTER_FMT(_logger, "write to mbedtls socket=0x%012Ix, queue_size=%zu",
			PTR_VAL(std::addressof(socket)),
			size()
		);

		written = 0;
		snd_status snd_status{ snd_status_code::NETCTX_SND_OK, 0, 0 };

		while (!is_empty() && snd_status.code == snd_status_code::NETCTX_SND_OK) {
			// Get the next contiguous block of data.
			const PBufQueue::cblock data_cblock{ get_cblock() };

			// Send this block.
			snd_status = socket.send_data(data_cblock.pdata, data_cblock.len);

			if (snd_status.code == snd_status_code::NETCTX_SND_OK) {
				// Move the pointer into the queue if bytes have been sent.
				if (!move(snd_status.sbytes)) {
					_logger->error("INTERNAL ERROR: OutputQueue::move failed");
					snd_status.code = snd_status_code::NETCTX_SND_ERROR;
					snd_status.rc = MBEDTLS_ERR_NET_SOCKET_FAILED;
				}
				else
					written += snd_status.sbytes;
			}
		}

		LOG_TRACE(_logger, "written to mbedtls => status=%d rc=%d written=%zu, queue_size=%zu",
			snd_status.code,
			snd_status.rc,
			written,
			size()
		);

		if (snd_status.code == snd_status_code::NETCTX_SND_RETRY) {
			// The output buffer was full, we will try to send the pbuf chain later
			snd_status.rc = 0;
		}

		return snd_status.rc;
	}


	utl::lwip_err OutputQueue::write(struct tcp_pcb* socket, size_t& written)
	{
		TRACE_ENTER_FMT(_logger, "write to lwip socket=0x%012Ix, queue_size=%zu, sndbuf=%d, unsent=%d",
			PTR_VAL(std::addressof(socket)),
			size(),
			tcp_sndbuf(socket),
			socket->unsent != nullptr
		);

		written = 0;
		lwip_err rc = ERR_OK;

		while (!is_empty() && rc == ERR_OK) {
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
			rc = ::tcp_write(socket, data_cblock.pdata, data_cblock.len, flags);
			if (rc == ERR_OK) {
				// Move the pointer into the queue if bytes have been copied to the TCP queue.
				if (!move(data_cblock.len)) {
					_logger->error("INTERNAL ERROR: OutputQueue::move failed");
					rc = ERR_VAL;
				}
				else
					written += data_cblock.len;
			}
		}

		if (rc == ERR_OK && (written > 0 || socket->unsent)) {
			rc = ::tcp_output(socket);
		}

		LOG_TRACE(_logger, "written to lwip => rc=%d written=%zu, queue_size=%zu",
			rc,
			written,
			size()
		);

		if (rc == ERR_IF) {
			// The output buffer was full, we will try to send the pbuf chain later
			rc = ERR_OK;
		}

		return rc;
	}

	const char* OutputQueue::__class__ = "OutputQueue";
}
