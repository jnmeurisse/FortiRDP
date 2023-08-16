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

#include "tools/Logger.h"

namespace net {
	using namespace tools;

	using SSLCtxPtr = std::unique_ptr<::SSL_CTX, decltype(&::SSL_CTX_free)>;
	using SSLPtr = std::unique_ptr<::SSL, decltype(&::SSL_free)>;



	class SslContext final
	{
	public:
		explicit SslContext();
		SslContext(const SslContext& context);
		~SslContext();

		SSLPtr create_ssl() const;

		/* Load the CA certificate from the give file.
		*/
		bool load_ca_cert(const std::string& filename);

		//bool load_user_cert(const std::string& filename, );

	private:
		// A reference to the application logger
		Logger* const _logger;

		// The SSL context
		const SSLCtxPtr _context;
	};

}
