/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <memory>
#include "mbedccl/x509.h"

namespace ccl {
	using x509crt_ptr = std::unique_ptr<::x509crt, decltype(&::x509crt_free)>;
}
