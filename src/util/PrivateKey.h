/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once
#include <memory>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/pk.h>
#include "util/ErrUtil.h"


namespace utl {

	class PrivateKey {
	public:
		explicit PrivateKey();
		~PrivateKey();

		/**
		 * Forbid copying the key.
		*/
		explicit PrivateKey(const PrivateKey& other) = delete;
		PrivateKey& operator=(const PrivateKey&) = delete;

		/**
		 * Loads the private key from the file.
		*/
		utl::mbed_err load(const char* filename, const char* passcode);

		/**
		* Returns a reference to the private key.
		*/
		mbedtls_pk_context& get_pk() { return _key; }

	private:
		mbedtls_ctr_drbg_context _ctr_drbg;
		mbedtls_pk_context _key;
	};

}
