/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <memory>
#include "mbedccl/pkctx.h"

namespace ccl {
	using pkctx_ptr = std::unique_ptr<::pkctx, decltype(&::pkctx_free)>;
}
