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

	CrtDigest::CrtDigest():
		_digest{ 0 }
	{
	}


	CrtDigest::CrtDigest(const X509* crt):
		CrtDigest()
	{
		if (crt) {
			X509_digest(crt, EVP_sha256(), _digest, nullptr);
		}
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