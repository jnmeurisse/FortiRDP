/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <stdexcept>
#include <string>


namespace http {

	/**
	 * Exception raised when the syntax of an HTTP cookie is incorrect.
	 */
	class CookieError final : public std::logic_error
	{
	public:
		explicit CookieError(const std::string msg);
	};

}
