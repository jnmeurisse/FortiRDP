/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "CrtDigest.h"

#include <cstring>
#include <mbedtls/sha256.h>


namespace fw {

	CrtDigest::CrtDigest() noexcept :
		_digest{ 0 }
	{
	}


	CrtDigest::CrtDigest(const mbedtls_x509_crt* crt) noexcept :
		CrtDigest()
	{
		if (crt)
			::mbedtls_sha256(crt->raw.p, crt->raw.len, _digest.data(), 0);
	}


	bool CrtDigest::operator== (const CrtDigest& other) const noexcept
	{
		return std::memcmp(_digest.data(), other._digest.data(), _digest.size()) == 0;
	}


	bool CrtDigest::operator!= (const CrtDigest& other) const noexcept
	{
		return !(*this == other);
	}

}
