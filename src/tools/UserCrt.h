/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once
#include <memory>
#include "tools/X509Crt.h"
#include "tools/PrivateKey.h"


namespace tools {
	using namespace tools;

	class UserCrt {
	public:
		UserCrt();
		~UserCrt();
		UserCrt(UserCrt& other) = delete;

		X509Crt crt;
		PrivateKey pk;
	};

	using UserCrtPtr = std::unique_ptr<UserCrt>;
}
