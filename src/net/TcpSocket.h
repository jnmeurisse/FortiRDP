/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "tools/Timer.h"
#include "net/Socket.h"


namespace net {
	using namespace tools;

	class TcpSocket : public Socket {
	public:
		explicit TcpSocket();
		explicit TcpSocket(netctx* ctx);
		virtual ~TcpSocket();

		/* Connects this socket to the specified end point.
		*/
		virtual mbed_errnum connect(const Endpoint& ep) override;

		virtual mbed_errnum close() override;

		virtual mbed_errnum set_nodelay(bool no_delay) override;

		virtual netctx_rcv_status recv(unsigned char* buf, size_t len) override;

		virtual netctx_snd_status send(const unsigned char* buf, size_t len) override;

		virtual netctx_rcv_status read(unsigned char* buf, size_t len, Timer& timer) override;

		virtual netctx_snd_status write(const unsigned char* buf, size_t len, Timer& timer) override;

		virtual int get_fd() const noexcept override;

	protected:
		netctx_poll_status poll(bool read, bool write, uint32_t timeout);
		virtual netctx_poll_status poll_rcv(uint32_t timeout);
		virtual netctx_poll_status poll_snd(uint32_t timeout);
	};

}