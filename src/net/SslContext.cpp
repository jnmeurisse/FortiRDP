/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "SslContext.h"

namespace net {
	static SSL_CTX* SSL_CTX_inc_ref(SSL_CTX* ctx)
	{
		if (ctx)
			SSL_CTX_up_ref(ctx);
		return ctx;
	}


	SslContext::SslContext() :
		_logger(Logger::get_logger()),
		_context(::SSL_CTX_new(::TLS_client_method()), &::SSL_CTX_free)
	{
		DEBUG_CTOR(_logger, "SslContext");

		// check for memory allocation error
		SSL_CTX* const ctx = _context.get();
		if (!ctx)
			throw std::bad_alloc();

		SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
		SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);
		::SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);

		::SSL_CTX_set_ciphersuites(
			ctx,
			"TLS_CHACHA20_POLY1305_SHA256:TLS_AES_256_GCM_SHA384:TLS_AES_128_GCM_SHA256");
		
		::SSL_CTX_set_default_passwd_cb(ctx, nullptr);
		::SSL_CTX_set_default_passwd_cb_userdata(ctx, this);
	}

	
	SslContext::SslContext(const SslContext& context) :
		_logger(Logger::get_logger()),
		_context(SSL_CTX_inc_ref(context._context.get()), &::SSL_CTX_free)
	{
		DEBUG_CTOR(_logger, "SslContext");
	}


	SslContext::~SslContext()
	{
		DEBUG_DTOR(_logger, "SslContext");
	}


	SSLPtr SslContext::create_ssl() const
	{
		return SSLPtr{ ::SSL_new(_context.get()), ::SSL_free };
	}
		

	bool SslContext::load_ca_cert(const std::string& filename)
	{
		DEBUG_ENTER(_logger, "SslContext", "load_ca_cert");

		return SSL_CTX_load_verify_file(_context.get(), filename.c_str()) == 1;
	}


/*	bool SslContext::load_user_cert(const std::string & filename)
	{
		
//		SSL_CTX_use_certificate_file();
		return false;
	}
*/
}
