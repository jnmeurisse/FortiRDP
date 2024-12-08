/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once
#include <memory>
#include <mbedtls/pk.h>
#include "tools/ErrUtil.h"


namespace tools {

	class PrivateKey {
	public:
		explicit PrivateKey();
		explicit PrivateKey(PrivateKey& other) = delete;
		~PrivateKey();

		/*
		* Loads the private key from the file.
		*/
		mbed_err load(const char* filename, const char* passcode);

		inline mbedtls_pk_context* get_pk() { return &_key; }

	private:
		mbedtls_pk_context _key;
	};


	using PrivateKeyPtr = std::unique_ptr<PrivateKey>;

}
