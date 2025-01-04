/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <memory>
#include <string>

#include "ccl/NetCtxPtr.h"

#include "net/Endpoint.h"

#include "tools/ErrUtil.h"
#include "tools/Timer.h"
#include "tools/Logger.h"

namespace net {

	using namespace tools;

	class Socket
	{
	public:
		virtual ~Socket();

		/* Connects this socket to the specified end point
		*/
		virtual mbed_errnum connect(const Endpoint& ep) = 0;

		/* Closes gracefully the socket
		*/
		virtual mbed_errnum close() = 0;

		/* Sets no delay option (disable Nagle algorithm)
		*/
		virtual mbed_errnum set_nodelay(bool no_delay) = 0;

		/* Receives data from the socket. The buffer pointed by the
		 * buf parameter will contain the data received. The maximum amount
		 * of data to be received at once is specified by len.
		 *
		 * The function returns the number of bytes received, which can
		 * be less than the number requested to be received. The function
		 * returns 0 if the socket has been closed, or a negative error code
		 * if an error has occurred.
		*/
		virtual netctx_rcv_status recv(unsigned char* buf, size_t len) = 0;

		/* Sends data to the socket. The buffer pointed by buf
		 * parameter must contain the data. The amount of data to sent is
		 * specified by the parameter len.
		 *
		 * The function returns the number of bytes sent, which can be less
		 * than the number requested to be sent. If an error has occurred,
		 * the function returns a negative error code.
		*/
		virtual netctx_snd_status send(const unsigned char* buf, size_t len) = 0;

		/* Reads a sequence of bytes from the socket. The buffer pointed by
		* the buffer parameter will contain the data received. The number of bytes to
		* read is specified by the len parameter.
		*
		* @param buf The buffer to store received data
		* @param len Number of bytes to read
		*
		*/
		virtual netctx_rcv_status read(unsigned char* buf, size_t len, Timer& timer) = 0;

		virtual netctx_snd_status write(const unsigned char* buf, size_t len, Timer& timer) = 0;

		/* Returns true if the socket is connected
		*/
		bool is_connected() const;

		/* Returns the socket descriptor
		*/
		virtual int get_fd() const = 0;

	protected:
		Socket(::netctx* netctx);
		inline netctx* get_ctx() const noexcept { return _netctx.get(); }

		// a reference to the application logger
		tools::Logger* const _logger;

	private:
		// the network context 
		const ccl::netctx_ptr _netctx;
	};
}
