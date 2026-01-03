/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "PrivateKey.h"


namespace utl {


	PrivateKey::PrivateKey()
	{
		mbedtls_ctr_drbg_init(&_ctr_drbg);
		mbedtls_pk_init(&_key);
	}


	PrivateKey::~PrivateKey()
	{
		mbedtls_pk_free(&_key);
	}


	mbed_err PrivateKey::load(const char* filename, const char* passcode)
	{
		mbedtls_pk_free(&_key);
		mbedtls_pk_init(&_key);

		return mbedtls_pk_parse_keyfile(&_key, filename, passcode, mbedtls_ctr_drbg_random, &_ctr_drbg);
	}


}