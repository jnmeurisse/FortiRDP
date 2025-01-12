/*!
* This file is part of FortiRDP
*
* Copyright (C) 2025 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <cstdint>
#include "tools/Timer.h"
#include "net/Socket.h"


namespace net {
	using namespace tools;

	class TcpSocket : public Socket {
	public:
		/* Constructs a TcpSocket.
		*/
		explicit TcpSocket();
		virtual ~TcpSocket();

		/* Connects this socket to the specified end point.
		*/
		virtual mbed_err connect(const Endpoint& ep) override;

		/* Closes gracefully the socket.
		*/
		virtual mbed_err close() override;

		/* Sets no delay option (disable Nagle algorithm)
		*/
		virtual mbed_err set_nodelay(bool no_delay) override;

		/* Receives data from the socket.
		 * See base class.
		*/
		virtual netctx_rcv_status recv(unsigned char* buf, size_t len) override;

		/* Sends data from the socket.
		 * See base class.
		*/
		virtual netctx_snd_status send(const unsigned char* buf, size_t len) override;

		/* Reads data from the socket.
		 * See base class.
		*/
		virtual netctx_rcv_status read(unsigned char* buf, size_t len, Timer& timer) override;

		/* Writes data to the socket.
		 * See base class.
		*/
		virtual netctx_snd_status write(const unsigned char* buf, size_t len, Timer& timer) override;

		/* Returns the socket file descriptor.
		 *
		 * The function returns -1 if the socket is not connected.
		*/
		virtual int get_fd() const noexcept override;

	protected:
		/* Constructs a socket from a network context.
		*/
		explicit TcpSocket(mbedtls_net_context* ctx);

		/*
		 * Checks and waits for the context to be ready for reading and/or writing 
		 * data.  The function waits until data is available when waiting for read 
		 * or waits that data have been send when waiting for write or a timeout
		 * occurred.
		 * 
		 * return a netctx_poll_status
		*/
		netctx_poll_status poll(bool read, bool write, uint32_t timeout);

		/*
		 * Checks and waits for the context to be ready for reading data.  The
		 * function waits until data is available or a timeout occurred.
		 *
		 * return a netctx_poll_status
		*/
		virtual netctx_poll_status poll_rcv(uint32_t timeout);

		/*
		 * Checks and waits for the context to be ready for writing data.  The
		 * function waits data have been send or a timeout occurred.
		 *
		 * return a netctx_poll_status
		*/
		virtual netctx_poll_status poll_snd(uint32_t timeout);
	};

}