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
	 * Exception raised when an error is detected in an HTTP answer.
	 */
	class http_error final : public tools::frdp_error
	{
	public:
		using tools::frdp_error::frdp_error;
	};

}
