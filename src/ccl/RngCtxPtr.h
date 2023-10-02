/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <memory>
#include "mbedccl/rngctx.h"

namespace ccl {
	using rngctx_ptr = std::unique_ptr<::rngctx, decltype(&::rngctx_free)>;
}
