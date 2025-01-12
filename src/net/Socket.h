/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <memory>
#include <cstdint>

#include "net/NetContext.h"
#include "net/Endpoint.h"

#include "tools/ErrUtil.h"
#include "tools/Timer.h"
#include "tools/Logger.h"


namespace net {
	using netctx_ptr = std::unique_ptr<struct mbedtls_net_context, decltype(&netctx_free)>;
	using namespace tools;

	/**
	* Socket  - an abstract socket.
	*
	*/
	class Socket
	{
	public:
		virtual ~Socket();

		/* Connects this socket to the specified end point.
		*/
		virtual mbed_err connect(const Endpoint& ep) = 0;

		/* Closes gracefully the socket.
		*/
		virtual mbed_err close() = 0;

		/* Sets no delay option (disable Nagle algorithm)
		*/
		virtual mbed_err set_nodelay(bool no_delay) = 0;

		/* Receives data from the socket. The buffer pointed by the
		 * buf parameter will contain the data received. The maximum amount
		 * of data to be received at once is specified by len.
		 *
		 * The function returns a netctx_rcv_status.
		*/
		virtual netctx_rcv_status recv(unsigned char* buf, size_t len) = 0;

		/* Sends data to the socket. The buffer pointed by buf
		 * parameter must contain the data. The amount of data to sent is
		 * specified by the parameter len.
		 *
		 * The function returns a netctx_snd_status.
		*/
		virtual netctx_snd_status send(const unsigned char* buf, size_t len) = 0;

		/* Reads a sequence of bytes from the socket. The buffer pointed by
		 * the buffer parameter will contain the data received. The number of bytes to
		 * read is specified by the len parameter.  The function stops reading when the
		 * given timer is elapsed.
		 *
		 * @param buf The buffer to store received data.
		 * @param len Number of bytes to read.
		 * @param timer Maximal amount of time to wait before returning.
		 *
		 * @return a netctx_rcv_status.
		 *
		*/
		virtual netctx_rcv_status read(unsigned char* buf, size_t len, tools::Timer& timer) = 0;

		/* Writes a sequence of bytes to the socket. The buffer pointed by buf
		 * parameter must contain the data. The amount of data to sent is
		 * specified by the parameter len.  The function stops writing when the
		 * given timer is elapsed.
		 *
		 * @param buf The buffer storing send data.
		 * @param len Number of bytes to write.
		 * @param timer Maximal amount of time to wait before returning.
		 *
		 * @return a netctx_snd_status.
		 *
		 */
		virtual netctx_snd_status write(const unsigned char* buf, size_t len, tools::Timer& timer) = 0;

		/* Returns true if the socket is connected.
		*/
		bool is_connected() const;

		/* Returns the socket file descriptor.
		 * 
		 * The function returns -1 if the socket is not connected.
		*/
		virtual int get_fd() const = 0;

	protected:
		/* Allocates a socket from a network context
		*/
		Socket(mbedtls_net_context* netctx);

		/* Returns the network context.
		*/
		inline mbedtls_net_context* get_ctx() const noexcept { return _netctx.get(); }

		// A reference to the application logger.
		tools::Logger* const _logger;

	private:
		// The network context.
		const netctx_ptr _netctx;
	};

}
