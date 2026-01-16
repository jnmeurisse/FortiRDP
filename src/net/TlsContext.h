/*!
* This file is part of FortiRDP
*
* Copyright (C) 2025 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>

#include "net/Socket.h"
#include "util/ErrUtil.h"


namespace net {
	/**
	 * @enum hdk_status_code
	 * Enum representing the status codes for the TLS handshake.
	 *
	 * This enum defines different states or outcomes during the TLS handshake process.
	 * - `SSLCTX_HDK_OK`        : Indicates the handshake was successful.
	 * - `SSLCTX_HDK_ERROR`     : Indicates an error occurred during the handshake.
	 * - `SSLCTX_HDK_WAIT_IO`   : Indicates that the handshake is waiting for I/O operations.
	 * - `SSLCTX_HDK_WAIT_ASYNC`: Indicates that the handshake is waiting for asynchronous
	 *                            operations to complete.
	*/
	enum class hdk_status_code {
		SSLCTX_HDK_OK,
		SSLCTX_HDK_ERROR,
		SSLCTX_HDK_WAIT_IO,
		SSLCTX_HDK_WAIT_ASYNC,
	};

	/**
	 * @struct tls_handshake_status
	 * Struct holding the status of the TLS handshake.
	 *
	 * This struct represents the result of a TLS handshake operation. It includes:
	 * - `status_code` : The status of the handshake (based on the `hdk_status_code` enum).
	 * - `rc`          : A return code that provides additional information, such as
	 *                   error codes or operation success codes.
	 * Possible combinations :
	 *     code        | HDK_ERROR   | HDK_OK     | HDK_WAIT_IO   | HDK_WAIT_ASYNC
	 *     rc          | error code  |    0       | r/w bit mask  | error code
	 */
	struct tls_handshake_status {
		hdk_status_code status_code;
		int rc;
	};

	/**
	 * @enum close_status_code
	 * Enum representing the status codes for the TLS close notify.
	 *
	 * This enum defines different the outcome of the close notify operation.
	 * - `SSLCTX_CLOSE_OK`      : Indicates the close notify was successful.
	 * - `SSLCTX_CLOSE_ERROR`   : Indicates an error occurred during the close notify.
	 * - `SSLCTX_CLOSE_RETRY`   : Indicates that the close notify operation should be retried.
	*/
	enum class close_status_code {
		SSLCTX_CLOSE_OK,
		SSLCTX_CLOSE_ERROR,
		SSLCTX_CLOSE_RETRY
	};

	/**
	 * @struct tls_close_status
	 * Struct holding the status of the TLS close notify.
	 *
	 * This struct represents the result of a TLS close notify operation. It includes:
	 * - `status_code` : The status of the close notify.
	 * - `rc`          : A return code that provides additional information, such as
	 *                   error codes or operation success codes.
	 * Possible combinations :
	 *     code        | CLOSE_ERROR | CLOSE_OK   | CLOSE_RETRY
	 *     rc          | error code  |    0       | r/w bit mask
	 */
	struct tls_close_status {
		close_status_code status_code;
		int rc;
	};


	class TlsContext {
	public:
		/**
		 * Constructs and initialize a TlContext.
		*/
		TlsContext();

		/**
		 * Destroys a TlsContext object.
		 *
		 * The destructor ensures all resources are released.
		*/
		~TlsContext();
		
		/**
		 * Configures the TLS context with the specified SSL configuration and network context.
		 *
		 * This function sets up the BIO callbacks for the TLS context, linking it to the
		 * provided network context for sending and receiving data. It then applies the
		 * given SSL configuration to the context, completing its setup.
		 *
		 * @param config The SSL configuration to be applied to the TLS context.
		 * @param netctx The network context used for I/O operations in the TLS handshake
		 *               and secure communication.
		 * @return mbed_err The result of the configuration process. Returns 0 on success
		 *                  or a specific error code from the mbedTLS library on failure.
		 */
		utl::mbed_err configure(const mbedtls_ssl_config& config, mbedtls_net_context& netctx);

		/**
		 * Cleans up the TLS context by freeing its resources.
		 *
		 * This function releases any resources allocated to the TLS context.  After
		 * calling this function, the context is ready for reconfiguration via the
		 * `configure` method.
		 */
		void clear();

		/**
		 *  Sets host name to check against the received server certificate.
		 * 
		 */
		utl::mbed_err set_hostname(const std::string& hostname);

		/**
		 * Notifies the peer that the connection is being closed.
		 *
		 */
		net::tls_close_status close_notify();

		/**
		 * Performs the TLS handshake.
		 *
		 * The function must be called multiple times until the handshake is complete
		 * or an error occurs.
		 *
		 */
		net::tls_handshake_status handshake();

		/**
		 * Receives data from the network context configured for this object.
		 *
		 * The received data will be stored in the buffer pointed to by the `buf` parameter,
		 * and the `len` parameter specifies the size of the buffer.
		 *
		 * This function returns a `rcv_status` value, which indicates the result of the
		 * receive operation.
		*/
		net::rcv_status recv_data(unsigned char* buf, size_t len);

		/**
		 * Transmits data through the network context set during object configuration.
		 *
		 * The `buf` parameter must point to the data to be transmitted, and the `len`
		 * parameter specifies the size of the data to be sent.
		 *
		 * This method returns a `snd_status` value, which indicates the result of the
		 * send operation.
		 */
		net::snd_status send_data(const unsigned char* buf, size_t len);

		/**
		 * Returns the result of the certificate verification.
		 *
		 * The verification process takes place during the open. The result
		 * of this method is undefined until the connect method has been
		 * executed.
		*/
		utl::mbed_err get_crt_check() const;

		/**
		 * Returns the cipher suite selected to encrypt the TLS communication.
		*/
		std::string get_ciphersuite() const;

		/**
		 * Returns the TLS version.
		*/
		std::string get_tls_version() const;

		/**
		 * Returns a pointer to the X509 certificate received from the TLS server.
		 *
		 * The peer certificate is obtained during the connection.  This pointer
		 * remains valid until the context is cleared.
		*/
		const mbedtls_x509_crt* get_peer_crt() const;

	private:
		mbedtls_ssl_context _sslctx;
	};

}
