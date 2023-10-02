/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <memory>
#include "mbedccl/sslcfg.h"

namespace ccl {
	using sslcfg_ptr = std::unique_ptr<::sslcfg, decltype(&::sslcfg_free)>;
}
