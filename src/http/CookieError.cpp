/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "CookieError.h"


namespace http {

	CookieError::CookieError(const std::string msg) :
		logic_error(msg)
	{
	}
}