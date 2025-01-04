/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once
#include <functional>

#include "tools/Logger.h"
#include "tools/Path.h"

#include "ccl/SslCfgPtr.h"
#include "ccl/SslCtxPtr.h"
#include "ccl/RngCtxPtr.h"
#include "ccl/X509CrtPtr.h"


namespace net {
	using namespace tools;
	using ask_password_fn = std::function<bool(std::string&)>;


	class SslConfig final
	{
	public:
		explicit SslConfig();
		~SslConfig();

		::sslctx* create_ssl_context() const;

		/* Load the CA certificate from the give file into this configuration
		*/
		bool load_ca_cert(const Path& cert_filenam);

		bool load_user_cert(const std::string& filename, ask_password_fn ask_password);

	private:
		// A reference to the application logger
		Logger* const _logger;

		// A random number generator
		const ccl::rngctx_ptr _rng_context;

		// The SSL configuration
		const ccl::sslcfg_ptr _ssl_config;

		// The CA cert chain
		ccl::x509crt_ptr _x509_crt;
	};

}
