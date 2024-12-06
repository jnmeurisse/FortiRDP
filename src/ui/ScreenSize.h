/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

namespace ui {

	struct ScreenSize {
		static const int max_height = 32766;
		static const int max_width = 32766;

		int height;
		int width;

		bool is_valid() const {
			return (height >= 0 && height <= ScreenSize::max_height && width >= 0 && width <= ScreenSize::max_width);
		}
	};

}
