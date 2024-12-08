/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/pk.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>
#include "net/Socket.h"
#include "net/Endpoint.h"
#include "tools/ErrUtil.h"


namespace net {

	using namespace tools;

	/*
	 * The client side of a network TLS socket
	*/
	class TlsSocket : public Socket
	{
	public:
		TlsSocket();
		virtual ~TlsSocket();

		/* Defines the CA certificate
		*/
		void set_ca_crt(mbedtls_x509_crt* ca_crt);

		/* Returns the CA certificate
		*/
		const mbedtls_x509_crt* get_ca_crt() const;

		/* Defines the client certificate
		*/
		mbed_err set_user_crt(mbedtls_x509_crt* own_crt, mbedtls_pk_context *own_key);

		/* Initiates a connection to the specified endpoint.
		 *
		 * @param  ep Then endpoint to connect to
		*/
		mbed_err connect(const Endpoint& ep);

		/* Returns the result of the certificate verification.
		 *
		 * The verification process takes place during the open. The result
		 * of this method is undefined until the connect method has been
		 * executed.
		*/
		mbed_err get_crt_check() const;

		/* Returns the cipher suite selected to encrypt ssl communication
		*/
		std::string get_ciphersuite() const;

		/* Returns the TLS version
		*/
		std::string get_tls_version() const;

		/* Returns a pointer to the X509 certificate of the ssl server. The peer
		 * certificate is obtained during the connection.
		*/
		const mbedtls_x509_crt* get_peer_crt() const;

		/* Receives data from the socket.
		 * See Socket::recv
		*/
		virtual int recv(unsigned char* buf, const size_t len) override;

		/* Sends data to the socket.
		 * See Socket::send
		*/
		virtual int send(const unsigned char* buf, const size_t len) override;

		/* Flushes data
		 * See Socket::flush
		*/
		virtual mbed_err flush() override;

	protected:
		// SSL configurations
		mbedtls_entropy_context _entropy_ctx;
		mbedtls_ctr_drbg_context _ctr_drbg;
		mbedtls_ssl_config _ssl_config;

		// the ssl socket
		mbedtls_ssl_context _ssl_context;

		virtual void do_close() override;
	};

}