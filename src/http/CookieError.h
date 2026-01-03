/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "util/ErrUtil.h"


namespace http {

	/**
	 * Exception raised when the syntax of an HTTP cookie is incorrect.
	 */
	class cookie_error final : public utl::frdp_error
	{
	public:
		using utl::frdp_error::frdp_error;
	};

}
