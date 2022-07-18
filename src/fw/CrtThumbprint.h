/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "mbedtls\x509_crt.h"

namespace fw {

	/**
	 * Stores the thumbprint of a X.509 certificate. The thumbprint contains
	 * a sha256 hash of the certificate.
	 */
	class CrtThumbprint
	{
	public:
		/* Creates an undefined thumbprint
		*/
		CrtThumbprint();

		/* Creates the thumbprint from the specified certificate.
		 *
		 * @param crt The certificate from which we determine the thumbprint
		 */
		explicit CrtThumbprint(const mbedtls_x509_crt* crt);

		/* Compares for equality this thumbprint with another.
		 *
		 * @param other The other thumbprint to compare
		 * @return true if both thumbprints are equal
		 */
		bool operator== (const CrtThumbprint& other) const;

		/* Compares for inequality this thumbprint with another.
		 *
		 * @param other The other thumbprint to compare
		 * @return true if both thumbprints are different
		 */
		bool operator!= (const CrtThumbprint& other) const;

	private:
		// A SHA256 hash of a certificate
		unsigned char _thumbprint[32];
	};
}