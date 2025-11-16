/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <cstdint>
#include <mbedtls/net_sockets.h>

#include "net/Endpoint.h"
#include "tools/ErrUtil.h"
#include "tools/Timer.h"
#include "tools/Logger.h"


namespace net {
	using namespace tools;

	enum net_protocol {
		NETCTX_PROTO_TCP = MBEDTLS_NET_PROTO_TCP,
		NETCTX_PROTO_UDP = MBEDTLS_NET_PROTO_UDP
	};

	/**
	 * @enum rcv_status_code
	 * Enumerates the possible status codes for receiving data.
	 *
	 * This enumeration represents the outcome of a data-receive operation.
	 * - `NETCTX_RCV_ERROR` : Indicates that an error occurred during the receive operation.
	 * - `NETCTX_RCV_OK`    : Indicates that data was successfully received.
	 * - `NETCTX_RCV_RETRY` : Indicates that the receive operation should be retried.
	 * - `NETCTX_RCV_EOF`   : Indicates that the end of the socket has been closed.
	 */
	enum class rcv_status_code {
		NETCTX_RCV_ERROR,
		NETCTX_RCV_OK,
		NETCTX_RCV_RETRY,
		NETCTX_RCV_EOF
	};

	/**
	 * @struct rcv_status
	 * Represents the result of a data-receive operation.
	 *
	 * This structure encapsulates the details of a data-receive operation.
	 * - `code`  : A `rcv_status_code` value indicating the status of the receive operation.
	 * - `rc`    : An integer representing the return code of the operation, providing
	 *             additional information in case of errors or specific conditions.
	 * - `rbytes`: The number of bytes successfully received during the operation.
	 *
	 * Possible combinations :
	 *     code        | RCV_OK  | RCV_RETRY     | RCV_ERROR   | RCV_EOF
	 *     rc          |  = 0    | r/w bit mask  |  error code |    = 0
	 *     rbytes      |  > 0    | = 0           |  = 0        |    = 0
	 *
	 */
	struct rcv_status {
		rcv_status_code code;
		int rc;
		size_t rbytes;
	};

	/**
	 * @enum rcv_status_code
	 * Enumerates the possible status codes for receiving data.
	 *
	 * This enumeration represents the outcome of a data-send operation.
	 * - `NETCTX_SND_ERROR` : Indicates that an error occurred during the send operation.
	 * - `NETCTX_SND_OK`    : Indicates that data was successfully sent.
	 * - `NETCTX_SND_RETRY` : Indicates that the send operation should be retried.
	 */
	enum snd_status_code {
		NETCTX_SND_ERROR,
		NETCTX_SND_OK,
		NETCTX_SND_RETRY
	};

	/**
	 * @struct snd_status
	 * Represents the result of a data-send operation.
	 *
	 * This structure encapsulates the details of a data-send operation.
	 * - `code`  : A `snd_status_code` value indicating the status of the send operation.
	 * - `rc`    : An integer representing the return code of the operation, providing
	 *             additional information in case of errors or specific conditions.
	 * - `rbytes`: The number of bytes successfully sent during the operation.
	 *
	 * Possible combinations :
	 *     code        | SND_OK  | SND_RETRY     | SND_ERROR
	 *     rc          |  = 0    | r/w bit mask  |  error code
	 *     rbytes      |  > 0    | = 0           |  = 0
	 *
	 */
	struct snd_status {
		snd_status_code code;
		int rc;
		size_t sbytes;
	};


	/**
	* Socket  - an abstract socket.
	* Base class of TcpSocket and UdpSocket.
	*
	*/
	class Socket
	{
	public:
		/**
		 * Destroys a Socket object.
		 *
		 * Ensures the socket is closed and resources are released. If connected,
		 * the destructor terminates the connection and cleans up the underlying
		 * context to prevent resource leaks.
		 */
		virtual ~Socket();

		/**
		 * Initiates a connection to the specified endpoint.
		 *
		 * This function attempts to establish a connection to the given endpoint within
		 * the time specified by the `timer` parameter. If the connection process exceeds
		 * the remaining time on the timer, the connection is canceled, and the function
		 * returns a negative error code.
		 *
		 * @param ep The endpoint to connect to.
		 * @param protocol Specify the IP protocol (TCP or UPD).
		 * @param timer A timer specifying the timeout duration for the connection. If the
		 *              connection cannot be established within the timer's remaining time,
		 *              the operation is aborted.
		 *
		 * @return A status code indicating the success or failure of the connection attempt.
		 */
		virtual mbed_err connect(const Endpoint& ep, net::net_protocol protocol, const Timer& timer);

		/**
		 * Binds the listener to a specified endpoint.
		 *
		 * This function binds a listener to the provided endpoint, defined by a host name
		 * and port, and sets up the socket for non-blocking mode. The function ensures
		 * proper error handling and logging during the binding process.
		 *
		 * @param endpoint The endpoint to bind the listener to, specified by its host name
		 *                 and port.
		 *
		 * @return mbed_err 0 on success; a negative error code on failure.
		 */
		virtual mbed_err bind(const Endpoint& ep, net::net_protocol protocol);

		/**
		 * Closes the socket.
		*/
		virtual void close();

		/**
		 * Closes gracefully the socket.
		*/
		virtual void shutdown();

		/**
		 * Enables or disables the blocking mode.
		 *
		 * This function enables or disables the socket blocking mode.  The function
		 * must be called when the socket is connected.
		 *
		 * @return An error code of type `mbed_err` indicating the success or failure
		 *         of changing the mode.
		*/
		mbed_err set_blocking_mode(bool enable);

		/**
		 * Configures the no-delay option for the socket.
		 *
		 * This function enables or disables the Nagle algorithm, which controls the
		 * behavior of small packet transmission. When enabled (`no_delay` is `true`),
		 * the algorithm is disabled, allowing small packets to be sent immediately without
		 * waiting for more data. When disabled (`no_delay` is `false`), the algorithm is
		 * enabled, and small packets may be buffered until a larger packet can be sent.
		 * The function must be called when the socket is connected.
		 *
		 * @param no_delay If `true`, disables the Nagle algorithm (enables immediate
		 *                 sending of small packets). If `false`, enables the Nagle
		 *                 algorithm (buffers small packets).
		 *
		 * @return An error code of type `mbed_err` indicating the success or failure
		 *         of setting the option.
		 */
		mbed_err set_nodelay(bool no_delay);

		/**
		 * Receives data from the socket.
		 *
		 * The received data is stored in the buffer pointed to by the `buf` parameter.
		 * The `len` parameter specifies the maximum amount of data to be received in a
		 * single call.
		 *
		 * This function returns a value of type `rcv_status`, indicating the status
		 * of the receive operation.
		 */
		virtual net::rcv_status recv_data(unsigned char* buf, size_t len);

		/**
		 * Sends data to the socket.
		 *
		 * The buffer pointed to by the `buf` parameter must contain the data to be sent.
		 * The `len` parameter specifies the amount of data to send.
		 *
		 * This function returns a value of type `snd_status`, indicating the status
		 * of the send operation.
		 */
		virtual net::snd_status send_data(const unsigned char* buf, size_t len);

		/**
		 * Returns true if the socket is connected.
		*/
		inline bool is_connected() const noexcept { return get_fd() != -1; }

		/**
		 * Returns the socket file descriptor.
		 * 
		 * The function returns -1 if the socket is not connected.
		*/
		inline int get_fd() const noexcept { return _netctx.fd; }

		/**
		 * Retrieves the port number associated with the socket.
		 *
		 * This method determines the port number of the local endpoint of the socket,
		 * if the socket file descriptor is valid. It supports both IPv4 and IPv6
		 * address families.
		 *
		 * @return False if the socket is not bound or the file descriptor is invalid.
		 *
		 */
		bool get_port(uint16_t& port) const noexcept;

	protected:
		// A reference to the application logger.
		tools::Logger* const _logger;

		/* Allocates a disconnected socket.
		*/
		Socket();

		/* Returns the mbedtls network context.
		*/
		inline mbedtls_net_context* netctx()  noexcept { return &_netctx; }

		/**
		 * @enum poll_status_code
		 * Enumerates the possible status codes for a polling operation.
		 *
		 * This enumeration defines the outcomes of a polling operation:
		 * - `NETCTX_POLL_ERROR`  : Indicates that an error occurred during polling.
		 * - `NETCTX_POLL_OK`     : Indicates that the polling operation was successful.
		 * - `NETCTX_POLL_TIMEOUT`: Indicates that the polling operation timed out.
		 */
		enum class poll_status_code {
			NETCTX_POLL_ERROR,
			NETCTX_POLL_OK,
			NETCTX_POLL_TIMEOUT
		};

		/**
		 * @struct poll_status
		 * Represents the result of a polling operation.
		 *
		 * This structure contains details about the outcome of a polling operation:
		 * - `code`: A `poll_status_code` value that specifies the polling status.
		 * - `rc`: An integer providing a return code related to the polling operation.
		 *
		 * Possible combinations :
		 *     code        POLL_ERROR   |    POLL_OK     | POLL TIMEMOUT
		 *     rc          error code   |  r/w bit mask  | error code
		 */
		struct poll_status {
			poll_status_code code;
			int rc;
		};

		/**
		 * Checks and waits for the socket to be ready for reading and/or writing data.
		 *
		 * This function monitors the socket and waits for either read or write operations
		 * to be possible. If `read` is `true`, it waits until data is available for reading.
		 * If `write` is `true`, it waits until data can be sent. The function will also
		 * respect the specified `timeout`. If the timeout is reached before either
		 * condition is met, the function will return without performing the requested
		 * operation.
		 *
		 * @param rw Bit flags that indicate if the function waits for data to be available
		             for reading or writing.
		 * @param timeout The maximum amount of time (in milliseconds) to wait before
		 *                returning, regardless of whether the requested conditions are met.
		 *
		 * @return A value of type `poll_status`, indicating the status of the
		 *         polling operation (e.g., success, timeout, etc.).
		 */
		virtual net::Socket::poll_status poll(int rw, uint32_t timeout);

		/**
		 * Accepts a new connection on the current socket and assigns it to the client socket.
		 *
		 * This method listens for an incoming connection on the current socket and, upon
		 * success, initializes the provided `client_socket` with the connection details.
		 * It ensures that the current socket is in a connected state and the `client_socket`
		 * is not already connected.
		 *
		 * @param client_socket Reference to the `Socket` object where the accepted connection
		 *                      will be stored.
		 * @return mbed_err Returns 0 on success, or an error code indicating failure:
		 *                  - `MBEDTLS_ERR_NET_INVALID_CONTEXT` if the current socket is
		 *                    not connected or the client socket is already connected.
		 *                  - For other error codes, see mbedtls library documentation.
		 */
		virtual mbed_err accept(Socket& client_socket);

	private:
		// The class name.
		static const char* __class__;

		// The network context.
		mbedtls_net_context _netctx;
	};

}
