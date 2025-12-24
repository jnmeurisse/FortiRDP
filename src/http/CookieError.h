/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "tools/ErrUtil.h"


namespace http {

	/**
	 * Exception raised when the syntax of an HTTP cookie is incorrect.
	 */
	class CookieError final : public tools::frdp_error
	{
	public:
		using tools::frdp_error::frdp_error;
	};

}
