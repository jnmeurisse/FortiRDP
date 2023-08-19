/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <string>

#include "CrtDigest.h"
#include "mbedtls\sha256.h"

namespace fw {

	CrtDigest::CrtDigest()
	{
		std::memset(_digest, 0, sizeof(_digest));
	}


	CrtDigest::CrtDigest(const mbedtls_x509_crt* crt)
	{
		if (crt)
			mbedtls_sha256_ret(crt->raw.p, crt->raw.len, _digest, 0);
		else
			std::memset(_digest, 0, sizeof(_digest));
	}


	bool CrtDigest::operator== (const CrtDigest& other) const
	{
		return std::memcmp(_digest, other._digest, sizeof(_digest)) == 0;
	}


	bool CrtDigest::operator!= (const CrtDigest& other) const
	{
		return !(*this == other);
	}
}