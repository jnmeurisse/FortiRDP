/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "net/TcpSocket.h"
#include "net/Endpoint.h"
#include "net/SslConfig.h"
#include "tools/Timer.h"

namespace net {
	using namespace tools;

	/*
	 * The client side of a network TLS socket
	*/
	class TlsSocket : public TcpSocket
	{
	public:
		TlsSocket(const SslConfig& config);
		virtual ~TlsSocket();

		virtual mbed_errnum connect(const Endpoint& ep) override;

		/* Performs the SSL handshake.
		*/
		sslctx_handshake_status handshake(Timer& timer);

		virtual mbed_errnum close() override;

		//TODO: add reset() !!

		/* Returns the result of the certificate verification.
		 *
		 * The verification process takes place during the open. The result
		 * of this method is undefined until the connect method has been
		 * executed.
		*/
		uint32_t get_crt_check() const;

		/* Returns the cipher suite selected to encrypt ssl communication
		*/
		std::string get_ciphersuite() const;

		/* Returns the TLS version.
		*/
		std::string get_tls_version() const;

		/* Returns a pointer to the X509 certificate of the SSL server. The peer
		 * certificate is obtained during the connection.
		*/
		const ::x509crt* get_peer_crt() const;

		/* Receives data from the socket.
		 * See Socket::recv
		*/
		virtual netctx_rcv_status recv(unsigned char* buf, size_t len) override;

		/* Sends data to the socket.
		 * See Socket::send
		*/
		virtual netctx_snd_status send(const unsigned char* buf, size_t len) override;

	protected:
		virtual netctx_poll_status poll_rcv(uint32_t timeout) override;
		virtual netctx_poll_status poll_snd(uint32_t timeout) override;

	private:
		// the SSL connection context
		const ccl::sslctx_ptr _ssl_context;

	};

}