/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <mbedtls/x509_crt.h>
#include <array>


namespace fw {

	/**
	 * Stores the digest of a X.509 certificate. The digest contains
	 * a sha256 hash of the certificate.
	 */
	class CrtDigest final
	{
	public:
		/* Creates an undefined digest
		*/
		CrtDigest() noexcept;

		/* Creates the digest from the specified certificate.
		 *
		 * @param crt The certificate from which we determine the digest
		 */
		explicit CrtDigest(const mbedtls_x509_crt* crt) noexcept;

		/* Compares for equality this digest with another.
		 *
		 * @param other The other digest to compare
		 * @return true if both digests are equal
		 */
		bool operator== (const CrtDigest& other) const noexcept;

		/* Compares for inequality this digest with another.
		 *
		 * @param other The other digest to compare
		 * @return true if both digests are different
		 */
		bool operator!= (const CrtDigest& other) const noexcept;

	private:
		// A SHA256 hash of a certificate
		std::array<unsigned char, 32> _digest;
	};

}
