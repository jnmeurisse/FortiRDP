/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <string>

#include "CrtDigest.h"


namespace fw {

	CrtDigest::CrtDigest()
	{
		std::memset(_digest, 0, sizeof(_digest));
	}


	CrtDigest::CrtDigest(const X509* crt)
	{
		if (crt) {
			X509_digest(crt, EVP_sha256(), _digest, nullptr);
		}
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