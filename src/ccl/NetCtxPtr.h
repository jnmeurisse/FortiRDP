/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <memory>
#include "mbedccl/netctx.h"

namespace ccl {
	using netctx_ptr = std::unique_ptr<::netctx, decltype(&::netctx_free)>;
}
