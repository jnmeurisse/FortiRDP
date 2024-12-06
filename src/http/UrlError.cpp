/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "UrlError.h"


namespace http {

	UrlError::UrlError(const std::string& msg) :
		std::logic_error(msg)
	{
	}

}
