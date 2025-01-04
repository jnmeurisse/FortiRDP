/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <string>

#include "CrtDigest.h"
#include "tools/ErrUtil.h"

namespace fw {

	CrtDigest::CrtDigest() :
		_digest{ 0 }
	{
	}

	
	CrtDigest::CrtDigest(const x509crt* crt) :
		CrtDigest()
	{
		if (crt) {
			const mbed_errnum errnum = x509crt_digest(crt, _digest, sizeof(_digest));
			if (errnum != 0)
				throw tools::mbed_error(errnum);
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
