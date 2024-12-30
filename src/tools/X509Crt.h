/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once
#include <memory>
#include <mbedtls/x509_crt.h>
#include "tools/ErrUtil.h"


namespace tools {

	class X509Crt {
	public:
		X509Crt();
		~X509Crt();
		X509Crt(X509Crt& other) = delete;
		
		/*
		* Loads one or more certificates and add them to the list of certificates.
		*/
		mbed_err load(const char* filename);

		/*
		* Writes an informational string about the certificate into the buffer.
		*/
		mbed_err get_info(char* buf, size_t size, const char* prefix) const;

		/*
		* Returns a pointer to the certificate chain
		*/
		inline mbedtls_x509_crt* get_crt() { return &_crt; }

	private:
		mbedtls_x509_crt _crt;
	};


	using X509crtPtr = std::unique_ptr<X509Crt>;

}