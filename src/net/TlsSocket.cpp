/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "net/TlsSocket.h"
#include "tools/StrUtil.h"


namespace net {
	using namespace tools;


	static int SSL_verify_cb(int preverify_ok, X509_STORE_CTX *x509_ctx)
	{
		return 1;
	}

	static void mbedtls_debug_fn(void *ctx, int level,
		const char *file, int line, const char *str)
	{
		((void)level);
		Logger* logger = (tools::Logger *)ctx;

		if (logger && strlen(str) > 1) {
			// remove \n from str
			std::string message{ str };
			message[message.size() - 1] = '\0';

			// get filename from whole path
			std::string path{ file };
			std::string filename;
			const size_t last_delim = path.find_last_of('\\');

			if (last_delim == std::wstring::npos) {
				filename = path;
			}
			else {
				filename = path.substr(last_delim + 1);
			}

			logger->trace(
				".... %s:%04d: %s", filename.c_str(), line, message.c_str());
		}
	}


	TlsSocket::TlsSocket(const SslContext& context) :
		Socket(BIO_new(BIO_f_ssl())),
		_ssl(context.create_ssl())
	{
		DEBUG_CTOR(_logger, "TlsSocket");
		SSL* const ssl = _ssl.get();
		if (!ssl)
			throw std::bad_alloc();

		//TODO: vérifier l'utilisation des modes
		SSL_clear_mode(ssl, SSL_MODE_AUTO_RETRY);

		::SSL_up_ref(ssl);
		::SSL_set_connect_state(ssl);
		BIO_set_ssl(get_bio(), ssl, BIO_CLOSE);

		BIO* conn_bio = ::BIO_new(BIO_s_connect());
		if (!conn_bio)
			throw std::bad_alloc();
		BIO_set_nbio(conn_bio, 1);
		::BIO_push(get_bio(), conn_bio);
	}


	TlsSocket::~TlsSocket()
	{
		DEBUG_DTOR(_logger, "TlsSocket");
	}


	bool TlsSocket::set_own_crt(const std::string& filename)
	{
		DEBUG_ENTER(_logger, "TlsSocket", "set_own_ctr");

		// return SSL_CTX_us	
		return true;
	}


	int TlsSocket::get_verify_result() const
	{
		return SSL_get_verify_result(_ssl.get());
	}


	std::string TlsSocket::get_ciphersuite() const
	{
		SSL_CIPHER const* cipher = SSL_get_current_cipher(_ssl.get());
		return  std::string(cipher ? SSL_CIPHER_get_name(cipher) : "not available");
	}


	std::string TlsSocket::get_tls_version() const
	{
		const char* const version = SSL_get_version(_ssl.get());
		return std::string(version ? version : "not available");
	}


	X509Ptr TlsSocket::get_peer_crt() const
	{
		return std::unique_ptr<::X509, decltype(&X509_free)>(
			::SSL_get1_peer_certificate(_ssl.get()),
			&X509_free
		);
	}


	bool TlsSocket::set_nodelay(bool no_delay)
	{
		return ::BIO_set_tcp_ndelay(get_fd(), no_delay ? 1 : 0) == 1;
	}


	int TlsSocket::get_fd() const noexcept
	{
		return _ssl.get() ? SSL_get_fd(_ssl.get()) : -1;
	}


	bool TlsSocket::do_connect(int timeout)
	{
		return 
			::SSL_set1_host(_ssl.get(), get_endpoint().hostname().c_str()) == 1 &&
			::BIO_do_connect_retry(get_bio(), timeout, 0) == 1;
	}
}
