/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <openssl/ssl.h>
#include <memory>
#include "net/Socket.h"
#include "net/SslContext.h"

namespace net {
	using namespace tools;

	using X509Ptr = std::unique_ptr<::X509, decltype(&X509_free)>;


	/*
	 * The client side of a network TLS socket
	*/
	class TlsSocket : public Socket
	{
	public:
		TlsSocket(const SslContext& context);
		virtual ~TlsSocket();

		/* Defines the client certificate
		*/
		bool set_own_crt(const std::string& filename);

		/* Returns the result of the certificate verification.
		 *
		 * The verification process takes place during the open. The result
		 * of this method is undefined until the connect method has been
		 * executed.
		*/
		int get_verify_result() const;

		/* Returns the cipher suite selected to encrypt ssl communication
		*/
		std::string get_ciphersuite() const;

		/* Returns the TLS version
		*/
		std::string get_tls_version() const;

		/* Returns a pointer to the X509 certificate of the ssl server. The peer
		 * certificate is obtained during the connection.
		*/
		X509Ptr get_peer_crt() const;

		virtual bool set_nodelay(bool no_delay) override;

		virtual int get_fd() const noexcept override;


		/* Flushes data
		 * See Socket::flush
		*/
		//virtual void flush() override;

	protected:
		virtual bool do_connect(int timeout) override;

	private:
		// - The SSL connection
		SSLPtr _ssl;
	};

}