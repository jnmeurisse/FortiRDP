/*!
* This file is part of FortiRDP
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Credential.h"
#include "StrUtil.h"

namespace utl {
	Credential::~Credential()
	{
		clear();
	}


	void Credential::clear() noexcept
	{
		username.clear();
		str::serase(password);
		password.clear();
	}
}
