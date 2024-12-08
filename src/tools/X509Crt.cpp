/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "X509Crt.h"


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

}
