/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include <mbedtls/pk.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>
#include "net/NetContext.h"
#include "net/TlsContext.h"
#include "net/TlsConfig.h"
#include "net/TcpSocket.h"
#include "net/Endpoint.h"
#

namespace net {
	using tlsctx_ptr = std::unique_ptr<struct mbedtls_ssl_context, decltype(&tlsctx_free)>;
	using namespace tools;


	/*
	 * The client side of a network TLS socket
	*/
	class TlsSocket : public TcpSocket
	{
	public:
		TlsSocket(const TlsConfig& tls_config);
		virtual ~TlsSocket();

		/* Enables or disables host name verification.
		 *
		 * Enabling host name verification ensures that the server's certificate matches
		 * the host name specified in the connect call, helping to prevent man-in-the-middle
		 * attacks. This must be configured before initiating the connection to the endpoint.
		 *
		 * @param enable If `true`, enables host name verification; if `false`, disables it.
		 */
		void set_hostname_verification(bool enable_verification);

		/* Initiates a connection to the specified endpoint.
		 * See base class.
		*/
		mbed_err connect(const Endpoint& ep, Timer& timer) override;

		/* Performs the SSL/TLS handshake.
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

		/* Close gracefully the socket.
		*/
		virtual mbed_err close() override;

		/* Return the result of the certificate verification.
		 *
		 * The verification process takes place during the open. The result
		 * of this method is undefined until the connect method has been
		 * executed.
		*/
		mbed_err get_crt_check() const;

		/* Return the cipher suite selected to encrypt the Tls communication.
		*/
		std::string get_ciphersuite() const;

		/* Return the TLS version.
		*/
		std::string get_tls_version() const;

		/* Return a pointer to the X509 certificate of the Tls server. The peer
		 * certificate is obtained during the connection.  This pointer remains
		 * valid until the socket is closed.
		*/
		const mbedtls_x509_crt* get_peer_crt() const;

		/* Receive data from the socket.
		 * See Socket::recv
		*/
		virtual netctx_rcv_status recv_data(unsigned char* buf, size_t len) override;

		/* Send data to the socket.
		 * See Socket::send
		*/
		virtual netctx_snd_status send_data(const unsigned char* buf, size_t len) override;

	protected:
		virtual netctx_poll_status poll_rcv(uint32_t timeout) override;
		virtual netctx_poll_status poll_snd(uint32_t timeout) override;

	private:
		// TLS configuration
		const TlsConfig& _tlscfg;
		bool _enable_hostname_verification;

		// the Tls socket
		const tlsctx_ptr _tlsctx;
	};

}
