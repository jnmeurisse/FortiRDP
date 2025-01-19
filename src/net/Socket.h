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
	* Base class of TcpSocket and UdpSocket.
	*
	*/
	class Socket
	{
	public:
		virtual ~Socket();

		/* Initiates a connection to the specified endpoint.
		 *
		 * This function attempts to establish a connection to the given endpoint within
		 * the time specified by the `timer` parameter. If the connection process exceeds
		 * the remaining time on the timer, the connection is canceled, and the function
		 * returns a negative error code.
		 *
		 * @param ep The endpoint to connect to.
		 * @param timer A timer specifying the timeout duration for the connection. If the
		 *              connection cannot be established within the timer's remaining time,
		 *              the operation is aborted.
		 *
		 * @return A status code indicating the success or failure of the connection attempt.
		 */
		virtual mbed_err connect(const Endpoint& ep, Timer& timer) = 0;

		/* Closes gracefully the socket.
		*/
		virtual mbed_err close() = 0;

		/* Configures the no-delay option for the socket.
		 *
		 * This function enables or disables the Nagle algorithm, which controls the
		 * behavior of small packet transmission. When enabled (`no_delay` is `true`),
		 * the algorithm is disabled, allowing small packets to be sent immediately without
		 * waiting for more data. When disabled (`no_delay` is `false`), the algorithm is
		 * enabled, and small packets may be buffered until a larger packet can be sent.
		 *
		 * @param no_delay If `true`, disables the Nagle algorithm (enables immediate
		 *                 sending of small packets). If `false`, enables the Nagle
		 *                 algorithm (buffers small packets).
		 *
		 * @return An error code of type `mbed_err` indicating the success or failure
		 *         of setting the option.
		 */
		virtual mbed_err set_nodelay(bool no_delay) = 0;

		/* Receives data from the socket.
		 *
		 * The received data is stored in the buffer pointed to by the `buf` parameter.
		 * The `len` parameter specifies the maximum amount of data to be received in a
		 * single call.
		 *
		 * This function returns a value of type `netctx_rcv_status`, indicating the status
		 * of the receive operation.
		 */
		virtual netctx_rcv_status recv_data(unsigned char* buf, size_t len) = 0;

		/* Sends data to the socket.
		 *
		 * The buffer pointed to by the `buf` parameter must contain the data to be sent.
		 * The `len` parameter specifies the amount of data to send.
		 *
		 * This function returns a value of type `netctx_snd_status`, indicating the status
		 * of the send operation.
		 */
		virtual netctx_snd_status send_data(const unsigned char* buf, size_t len) = 0;

		/* Reads a sequence of bytes from the socket.
		 *
		 * The function reads data and stores it in the buffer pointed to by the `buf` parameter.
		 * Reading continues until either the buffer is completely filled (as specified by the
		 * `len` parameter) or the specified `timer` elapses, whichever occurs first.
		 *
		 * @param buf Pointer to the buffer where received data will be stored.
		 * @param len The number of bytes to read (i.e., the size of the buffer).
		 * @param timer The maximum amount of time to wait for the operation to complete.
		 *
		 * @return A value of type `netctx_rcv_status` indicating the status of the
		 *         read operation.
		 */
		virtual netctx_rcv_status read(unsigned char* buf, size_t len, tools::Timer& timer) = 0;

		/* Writes a sequence of bytes to the socket.
		 *
		 * The function sends data from the buffer pointed to by the `buf` parameter to the socket.
		 * Writing continues until all bytes specified by the `len` parameter have been sent, or
		 * the specified `timer` elapses, whichever occurs first.
		 *
		 * @param buf Pointer to the buffer containing the data to be sent.
		 * @param len The number of bytes to send (i.e., the size of the data).
		 * @param timer The maximum amount of time to attempt the write operation.
		 *
		 * @return A value of type `netctx_snd_status` indicating the status of the
		 *         write operation.
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
