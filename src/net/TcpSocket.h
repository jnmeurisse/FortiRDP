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
		 * See base class.
		 *
		 *  Note: The MbedTLS network context has no option to specify a timeout.
		 * 	      https://github.com/Mbed-TLS/mbedtls/issues/8027
		 *
		*/
		virtual mbed_err connect(const Endpoint& ep, Timer& timer) override;

		/* Closes gracefully the socket.
		 * See base class.
		*/
		virtual mbed_err close() override;

		/* Sets no delay option.
		 * See base class.
		*/
		virtual mbed_err set_nodelay(bool no_delay) override;

		/* Receives data from the socket.
		 * See base class.
		*/
		virtual netctx_rcv_status recv_data(unsigned char* buf, size_t len) override;

		/* Sends data from the socket.
		 * See base class.
		*/
		virtual netctx_snd_status send_data(const unsigned char* buf, size_t len) override;

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

		/* Checks and waits for the context to be ready for reading and/or writing data.
		 *
		 * This function monitors the context and waits for either read or write operations
		 * to be possible. If `read` is `true`, it waits until data is available for reading.
		 * If `write` is `true`, it waits until data can be sent. The function will also
		 * respect the specified `timeout`. If the timeout is reached before either
		 * condition is met, the function will return without performing the requested
		 * operation.
		 *
		 * @param read If `true`, the function waits for data to be available for reading.
		 * @param write If `true`, the function waits for the ability to send data.
		 * @param timeout The maximum amount of time (in milliseconds) to wait before
		 *                returning, regardless of whether the requested conditions are met.
		 *
		 * @return A value of type `netctx_poll_status`, indicating the status of the
		 *         polling operation (e.g., success, timeout, etc.).
		 */
		netctx_poll_status poll(bool read, bool write, uint32_t timeout);

		/* Checks and waits for the context to be ready for reading data.
		 *
		 * This function waits until data is available for reading or until the specified
		 * `timeout` has elapsed.
		 *
		 * @param timeout The maximum amount of time (in milliseconds) to wait for data
		 *                to be available before returning.
		 *
		 * @return A value of type `netctx_poll_status`, indicating the status of the
		 *         polling operation (e.g., success, timeout, etc.).
		 */
		virtual netctx_poll_status poll_rcv(uint32_t timeout);

		/* Checks and waits for the context to be ready for writing data.
		 *
		 * This function waits until the context is ready to send data, or until the
		 * specified `timeout` has elapsed.
		 *
		 * @param timeout The maximum amount of time (in milliseconds) to wait for the
		 *                context to be ready for writing before returning.
		 *
		 * @return A value of type `netctx_poll_status`, indicating the status of the
		 *         polling operation (e.g., success, timeout, etc.).
		 */
		virtual netctx_poll_status poll_snd(uint32_t timeout);
	};

}
