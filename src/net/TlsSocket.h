/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include <mbedtls/x509_crt.h>
#include "net/TlsContext.h"
#include "net/TlsConfig.h"
#include "net/TcpSocket.h"
#include "net/Endpoint.h"


namespace net {
	using namespace tools;

	/**
	 * The client side of a network TLS socket
	*/
	class TlsSocket : public TcpSocket
	{
	public:
		/**
		 * Constructs a TlsSocket with the provided TLS configuration.
		 *
		 * This constructor initializes a TlsSocket instance using the given
		 * TlsConfig object, setting up the necessary parameters for secure
		 * communication over a TLS connection.
		 *
		 * @param tls_config The configuration object containing TLS settings.
		*/
		explicit TlsSocket(const TlsConfig& tls_config);

		/**
		 * Destroys a TlsSocket object.
		 * See base class.
		*/
		virtual ~TlsSocket();

		/**
		 * Enables or disables host name verification.
		 *
		 * Enabling host name verification ensures that the server's certificate
		 * matches the host name specified in the connect call, helping to prevent
		 * man-in-the-middle attacks. This flag must be configured before initiating
		 * the connection to the endpoint.
		 *
		 * @param enable If `true`, enables host name verification; if `false`, disables it.
		 */
		void set_hostname_verification(bool enable_verification);

		/**
		 * Initiates a connection to the specified endpoint.
		 * See base class.
		*/
		mbed_err connect(const Endpoint& ep, Timer& timer) override;

		/**
		 * Performs the SSL/TLS handshake.
		 *
		 * This function initiates the SSL/TLS handshake process to establish a secure
		 * connection. The handshake will proceed until it either completes successfully
		 * or the specified timeout (`timer`) is reached. If the handshake takes longer
		 * than the specified time, it will be canceled.
		 *
		 * @param timer A timer that specifies the timeout duration for the handshake process.
		 *              If the handshake is not completed within the given time, the operation
		 *              is canceled.
		 *
		 * @return A `tls_handshake_status` indicating the result of the handshake.
		 */
		tls_handshake_status handshake(Timer& timer);

		/**
		 * Closes gracefully the socket.
		*/
		virtual void shutdown() override;

		/**
		 * Returns the result of the certificate verification.
		 *
		 * The verification process takes place during the open. The result
		 * of this method is undefined until the connect method has been
		 * executed.
		*/
		mbed_err get_crt_check() const;

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
		 * valid until this socket is closed.
		*/
		const mbedtls_x509_crt* get_peer_crt() const;

		/**
		 * Returns the TLS configuration used to initialize the socket.
		 *
		 * @return A reference to the `TlsConfig` instance associated with
		 *         this socket.
		 */
		const TlsConfig& get_tls_config() const;

		/**
		 * Receives data from the socket.
		 * See Socket::recv_data
		*/
		virtual net::rcv_status recv_data(unsigned char* buf, size_t len) override;

		/**
		 * Sends data to the socket.
		 * See Socket::send_data
		*/
		virtual net::snd_status send_data(const unsigned char* buf, size_t len) override;

	private:
		// A reference to the TLS configuration.
		const TlsConfig& _tlscfg;

		// The TLS context.
		TlsContext _tlsctx;

		// True if the host name verification must be validated.
		bool _enable_hostname_verification;
	};

}
