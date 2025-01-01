/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "X509Crt.h"

#include <vector>
#include <mbedtls/base64.h>
#include <mbedtls/pem.h>



namespace tools {
	
	X509Crt::X509Crt()
	{
		mbedtls_x509_crt_init(&_crt);
	}

	
	X509Crt::~X509Crt()
	{
		mbedtls_x509_crt_free(&_crt);
	}


	mbed_err X509Crt::load(const char* filename)
	{
		return mbedtls_x509_crt_parse_file(&_crt, filename);
	}


	mbed_err X509Crt::get_info(char* buf, size_t size, const char* prefix) const
	{
		mbed_err errnum = mbedtls_x509_crt_info(buf, size, "   ", &_crt);

		return errnum >= 0 ? 0 : errnum;
	}


	bool X509Crt::to_string(std::string& pem) const
	{
		return X509crt_to_pem(&_crt, pem);
	}


	bool X509crt_to_pem(const mbedtls_x509_crt* crt, std::string& pem)
	{
		constexpr auto pem_begin_crt{ "-----BEGIN CERTIFICATE-----\n" };
		constexpr auto pem_end_crt{ "-----END CERTIFICATE-----\n" };
		size_t buffer_size{ 1024 };

		int ret = 0;
		if (crt) {
			do {
				size_t written{};
				std::vector<unsigned char> buffer(buffer_size);

				ret = mbedtls_pem_write_buffer(
					pem_begin_crt,
					pem_end_crt,
					crt->raw.p,
					crt->raw.len,
					&buffer[0],
					buffer.size(),
					&written
				);

				if (ret == 0)
					pem = std::string((char*)&buffer[0]);
				else if (ret == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL)
					buffer_size += 1024;
			} while (ret == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL);
		}

		return crt && (ret == 0);
	}

}
