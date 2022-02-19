/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <string>

#include "CrtThumbprint.h"
#include "mbedtls\sha256.h"

namespace fw {

	CrtThumbprint::CrtThumbprint()
	{
		std::memset(_thumbprint, 0, sizeof(_thumbprint));
	}


	CrtThumbprint::CrtThumbprint(const mbedtls_x509_crt& crt)
	{
		mbedtls_sha256_ret(crt.raw.p, crt.raw.len, _thumbprint, 0);
	}


	bool CrtThumbprint::operator== (const CrtThumbprint& other) const
	{
		return std::memcmp(_thumbprint, other._thumbprint, sizeof(_thumbprint)) == 0;
	}


	bool CrtThumbprint::operator!= (const CrtThumbprint& other) const
	{
		return !(*this == other);
	}
}