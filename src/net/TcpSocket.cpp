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
		DEBUG_CTOR(_logger);
	}


	TcpSocket::~TcpSocket()
	{
		DEBUG_DTOR(_logger);
	}


	mbed_err TcpSocket::connect(const Endpoint& ep, const Timer& timer)
	{
		if (_logger->is_debug_enabled())
			_logger->debug("... %x enter %s::%s ep=%s",
				(uintptr_t)this, 
				__class__,
				__func__,
				ep.to_string().c_str()
			);

		mbed_err rc = Socket::connect(ep, net_protocol::NETCTX_PROTO_TCP, timer);
		if (rc)
			goto terminate;

		rc = Socket::set_blocking_mode(false);
		if (rc)
			goto terminate;

	terminate:
		if (_logger->is_debug_enabled())
			_logger->debug("... %x leave %s::%s fd=%d rc=%d",
				(uintptr_t)this,
				__class__,
				__func__,
				get_fd(),
				rc
			);

		return rc;
	}


	net::rcv_status TcpSocket::read(unsigned char* buf, size_t len, const Timer& timer)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x enter %s::%s buffer=%x size=%zu",
				(uintptr_t)this,
				__class__,
				__func__,
				(uintptr_t)buf,
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
				".... %x leave %s::%s buffer=%x status=%d rc=%d len=%zu",
				(uintptr_t)this,
				__class__,
				__func__,
				(uintptr_t)buf,
				read_status.code,
				read_status.rc,
				read_status.rbytes
			);

		return read_status;
	}


	net::snd_status TcpSocket::write(const unsigned char* buf, size_t len, const Timer& timer)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x enter %s::%s buffer=%x size=%zu",
				(uintptr_t)this,
				__class__,
				__func__,
				(uintptr_t)buf,
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
				".... %x leave %s::%s buffer=%x status=%d rc=%d len=%zu",
				(uintptr_t)this,
				__class__,
				__func__,
				(uintptr_t)buf,
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
				".... %x enter %s::%s read=%x write=%d timeout=%lu",
				(uintptr_t)this,
				__class__,
				__func__,
				(rw & 1) != 0 ? 1 : 0,
				(rw & 2) != 0 ? 1 : 0,
				timeout
			);

		const poll_status status{ Socket::poll(rw, timeout) };

		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x leave %s::%s status=%d read=%x write=%d",
				(uintptr_t)this,
				__class__,
				__func__,
				status.code,
				(status.rc & 1) != 0 ? 1 : 0,
				(status.rc & 2) != 0 ? 1 : 0
			);

		return status;
	}

	const char* TcpSocket::__class__ = "TcpSocket";

};
