/*!
* This file is part of FortiRDP
*
* Copyright (C) 2025 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "net/TcpSocket.h"

namespace net {

	TcpSocket::TcpSocket() : 
		Socket()
	{
		DEBUG_CTOR(_logger, "TcpSocket");
	}


	TcpSocket::~TcpSocket()
	{
		DEBUG_DTOR(_logger, "TcpSocket");
	}


	mbed_err TcpSocket::connect(const Endpoint& ep, Timer& timer)
	{
		if (_logger->is_debug_enabled())
			_logger->debug("... %x enter TcpSocket::connect ep=%s",
				(uintptr_t)this, 
				ep.to_string().c_str()
			);

		// open a tcp socket.
		mbed_err rc = Socket::connect(ep, net_protocol::NETCTX_PROTO_TCP, timer);
		if (rc)
			goto terminate;

		// configure it as a non blocking socket.
		rc = Socket::set_blocking_mode(false);
		if (rc)
			goto terminate;

	terminate:
		if (_logger->is_debug_enabled())
			_logger->debug("... %x leave TcpSocket::connect fd=%d rc=%d",
				(uintptr_t)this, get_fd(),
				rc
			);

		return rc;
	}


	net::rcv_status TcpSocket::read(unsigned char* buf, size_t len, Timer& timer)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x enter TcpSocket::read buffer=%x size=%zu",
				(uintptr_t)this,
				buf,
				len
			);

		rcv_status read_status { rcv_status_code::NETCTX_RCV_ERROR, 0, 0 };

		do {
			const rcv_status rcv_data_status{ recv_data(buf, len) };

			buf += rcv_data_status.rbytes;
			len -= rcv_data_status.rbytes;

			read_status.code = rcv_data_status.code;
			read_status.rbytes += rcv_data_status.rbytes;
			read_status.rc = rcv_data_status.rc;

			if (rcv_data_status.code == rcv_status_code::NETCTX_RCV_RETRY) {
				const poll_status poll_rcv_status = poll(rcv_data_status.rc, timer.remaining_time());

				if (poll_rcv_status.code != poll_status_code::NETCTX_POLL_OK) {
					read_status.code = rcv_status_code::NETCTX_RCV_ERROR;
					read_status.rc = poll_rcv_status.rc;
				}
			}
		} while (len >= 0 && read_status.code == rcv_status_code::NETCTX_RCV_RETRY);

		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x leave TcpSocket::read buffer=%x status=%d rc=%d len=%zu",
				(uintptr_t)this,
				buf,
				read_status.code,
				read_status.rc,
				read_status.rbytes
			);

		return read_status;
	}


	net::snd_status TcpSocket::write(const unsigned char* buf, size_t len, Timer& timer)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x enter TcpSocket::write buffer=%x size=%zu",
				(uintptr_t)this,
				buf,
				len
			);

		snd_status write_status { NETCTX_SND_ERROR, 0, 0 };

		do {
			const snd_status snd_data_status{ send_data(buf, len) };

			buf += snd_data_status.sbytes;
			len -= snd_data_status.sbytes;

			write_status.code = snd_data_status.code;
			write_status.sbytes += snd_data_status.sbytes;
			write_status.rc = snd_data_status.rc;

			if (snd_data_status.code == NETCTX_SND_RETRY) {
				const poll_status poll_snd_status{ poll(snd_data_status.rc, timer.remaining_time()) };

				if (poll_snd_status.code != poll_status_code::NETCTX_POLL_OK) {
					write_status.code = snd_status_code::NETCTX_SND_ERROR;
					write_status.rc = poll_snd_status.rc;
				}
			}
		} while (len >= 0 && write_status.code == snd_status_code::NETCTX_SND_RETRY);

		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x leave TcpSocket::write buffer=%x status=%d rc=%d len=%zu",
				(uintptr_t)this,
				buf,
				write_status.code,
				write_status.rc,
				write_status.sbytes
			);

		return write_status;
	}


	net::Socket::poll_status TcpSocket::poll(int rw, uint32_t timeout)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x enter TcpSocket::poll read=%x write=%d timeout=%lu",
				(uintptr_t)this,
				(rw & 1) != 0 ? 1 : 0,
				(rw & 2) != 0 ? 1 : 0,
				timeout
			);

		const poll_status status{ Socket::poll(rw, timeout) };

		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x leave TcpSocket::poll status=%d read=%x write=%d",
				(uintptr_t)this,
				status.code,
				(status.rc & 1) != 0 ? 1 : 0,
				(status.rc & 2) != 0 ? 1 : 0
			);

		return status;
	}

};
