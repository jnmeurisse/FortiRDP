/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "net/SslConfig.h"

#include "ccl/PkCtxPtr.h"
#include "tools/SysUtil.h"
#include "tools/StrUtil.h"
#include "tools/ErrUtil.h"


namespace net {

	SslConfig::SslConfig() :
		_logger(Logger::get_logger()),
		_ssl_config(::sslcfg_alloc(), ::sslcfg_free),
		_rng_context(::rngctx_alloc(), ::rngctx_free),
		_x509_crt(::x509crt_alloc(), ::x509crt_free)
	{
		DEBUG_CTOR(_logger, "SslConfig");

		// Initialize the random number generator
		mbed_errnum errnum;
		if (!_rng_context.get())
			throw std::bad_alloc();

		errnum = ::rngctx_configure(_rng_context.get());
		if (errnum != 0)
			throw tools::mbed_error(errnum);

		// Initialize the SSL configuration
		if (!_ssl_config.get())
			throw std::bad_alloc();

		errnum = ::sslcfg_configure(_ssl_config.get(), _rng_context.get());
		if (errnum != 0)
			throw tools::mbed_error(errnum);

		if (_logger->is_trace_enabled()) {
			// enable debug for this configuration context
			::sslcfg_enable_debug(_ssl_config.get(), true);
		}
	}

	
	SslConfig::~SslConfig()
	{
		DEBUG_DTOR(_logger, "SslConfig");
	}


	::sslctx* SslConfig::create_ssl_context()  const
	{
		sslctx* const context = ::sslctx_alloc();
		if (!context)
			throw std::bad_alloc();

		mbed_errnum errnum = ::sslctx_configure(context, _ssl_config.get());
		//TODO: manage errnum

		return context;
	}


	bool SslConfig::load_ca_cert(const Path& cert_filename)
	{
		DEBUG_ENTER(_logger, "SslConfig", "load_ca_cert");

		// allocate a new X509 certificate
		_x509_crt.reset(x509crt_alloc());

		const std::wstring filename{ cert_filename.to_string() };
		const std::string compacted{ tools::wstr2str(cert_filename.compact(40)) };

		if (!tools::file_exists(filename)) {
			_logger->error("ERROR: can't find CA cert file %s", compacted.c_str());
			return false;
		}
		
		// Load the CA chain
		mbed_errnum errnum = 0;
		const int parse_file_rc = x509crt_parse_file(_x509_crt.get(), tools::wstr2str(filename).c_str());
		if (parse_file_rc < 0) {
			errnum = parse_file_rc;
			goto load_error;
		}
		
		// Associate the CA chain with this configuration.
		errnum = sslfcg_set_ca_chain(_ssl_config.get(), _x509_crt.get());
		if (errnum)
			goto load_error;

		_logger->info(">> CA cert loaded from file '%s'", compacted.c_str());
		return true;

	load_error:
		_logger->info("WARNING: failed to load CA cert file %s ", compacted.c_str());
		_logger->info("%s", tools::mbed_errmsg(errnum).c_str());
		return false;
	}


	bool SslConfig::load_user_cert(const std::string& filename, ask_password_fn ask_password)
	{
		ccl::pkctx_ptr own_key{ ::pkctx_alloc(), pkctx_free };
/*		mbed_err rc = mbedtls_pk_parse_keyfile(
			&own_key,
			tools::wstr2str(cert_files.crt_user_file.to_string()).c_str(),
			nullptr
		);
*/
		return false;
	}

}
