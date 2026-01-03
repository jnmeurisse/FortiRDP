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
#include "util/ErrUtil.h"


namespace aux {

	class X509Crt {
	public:
		X509Crt();
		~X509Crt();
		X509Crt(X509Crt& other) = delete;
		
		/**
		 * Loads one or more certificates and adds them to the list of certificates.
		*/
		mbed_err load(const char* filename);

		/**
		 * Writes an informational string about the certificate into the buffer.
		*/
		mbed_err get_info(char* buf, size_t size, const char* prefix) const;

		/**
		 * Returns a reference to the certificate chain.
		*/
		inline mbedtls_x509_crt& get_crt() { return _crt; }

	private:
		mbedtls_x509_crt _crt;
	};

	// A unique pointer to a x509 certificate
	using X509crtPtr = std::unique_ptr<X509Crt>;

	/**
	 * Converts a x509 certificate to PEM format.
	 * 
	 * @return True if the conversion succeeded, false if not.
	*/
	bool X509crt_to_pem(const mbedtls_x509_crt* crt, std::string& pem);

	/**
	 * Checks if the x509 is signed by a trusted CA stored in Windows.
	*/
	bool x509crt_is_trusted(const mbedtls_x509_crt* crt);

}
