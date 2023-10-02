/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <memory>
#include "mbedccl/sslctx.h"

namespace ccl {
	using sslctx_ptr = std::unique_ptr<::sslctx, decltype(&::sslctx_free)>;
}
