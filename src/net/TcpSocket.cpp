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
		_logger->debug("... 0x%012Ix enter %s::%s ep=%s",
			PTR_VAL(this),
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
		_logger->debug("... 0x%012Ix leave %s::%s fd=%d rc=%d",
			PTR_VAL(this),
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
				".... 0x%012Ix enter %s::%s buffer=0x%012Ix size=%zu",
				PTR_VAL(this),
				__class__,
				__func__,
				PTR_VAL(buf),
				len
			);

		rcv_status read_status { rcv_status_code::NETCTX_RCV_OK, 0, 0 };

		bool keep_reading = len > 0;
		while (keep_reading) {
			const rcv_status rcv_data_status =  recv_data(buf, len);

			read_status.code = rcv_data_status.code;
			read_status.rc = rcv_data_status.rc;

			if (read_status.code == rcv_status_code::NETCTX_RCV_OK) {
				// Update progress
				buf += rcv_data_status.rbytes;
				len -= rcv_data_status.rbytes;
				read_status.rbytes += rcv_data_status.rbytes;

				keep_reading = len > 0;
			}
			else if (read_status.code == rcv_status_code::NETCTX_RCV_RETRY) {
				// Handle the "Busy/Retry" case
				const poll_status poll_rcv_status = poll(read_status.rc, timer.remaining_time());

				if (poll_rcv_status.code != poll_status_code::NETCTX_POLL_OK) {
					read_status.code = rcv_status_code::NETCTX_RCV_ERROR;
					read_status.rc = poll_rcv_status.rc;
					keep_reading = false;
				}
			}
			else
				keep_reading = false;
		}

		if (_logger->is_trace_enabled())
			_logger->trace(
				".... 0x%012Ix leave %s::%s buffer=0x%012Ix status=%d rc=%d len=%zu",
				PTR_VAL(this),
				__class__,
				__func__,
				PTR_VAL(buf),
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
				".... 0x%012Ix enter %s::%s buffer=0x%012Ix size=%zu",
				PTR_VAL(this),
				__class__,
				__func__,
				PTR_VAL(buf),
				len
			);

		snd_status write_status { NETCTX_SND_OK, 0, 0 };

		bool keep_writing = len > 0;
		while (keep_writing) {
			const snd_status snd_data_status{ send_data(buf, len) };

			write_status.code = snd_data_status.code;
			write_status.rc = snd_data_status.rc;

			if (write_status.code == snd_status_code::NETCTX_SND_OK) {
				// Update progress
				buf += snd_data_status.sbytes;
				len -= snd_data_status.sbytes;
				write_status.sbytes += snd_data_status.sbytes;

				keep_writing = len > 0;
			}
			else if (write_status.code == NETCTX_SND_RETRY) {
				// Handle the "Busy/Retry" case
				const poll_status poll_snd_status{ poll(write_status.rc, timer.remaining_time()) };

				if (poll_snd_status.code != poll_status_code::NETCTX_POLL_OK) {
					write_status.code = snd_status_code::NETCTX_SND_ERROR;
					write_status.rc = poll_snd_status.rc;
					keep_writing = false;
				}
			}
			else
				keep_writing = false;
		}

		if (_logger->is_trace_enabled())
			_logger->trace(
				".... 0x%012Ix leave %s::%s buffer=0x%012Ix status=%d rc=%d len=%zu",
				PTR_VAL(this),
				__class__,
				__func__,
				PTR_VAL(buf),
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
				".... 0x%012Ix enter %s::%s read=%x write=%d timeout=%lu",
				PTR_VAL(this),
				__class__,
				__func__,
				(rw & 1) != 0 ? 1 : 0,
				(rw & 2) != 0 ? 1 : 0,
				timeout
			);

		const poll_status status{ Socket::poll(rw, timeout) };

		if (_logger->is_trace_enabled())
			_logger->trace(
				".... 0x%012Ix leave %s::%s status=%d read=%x write=%d",
				PTR_VAL(this),
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
