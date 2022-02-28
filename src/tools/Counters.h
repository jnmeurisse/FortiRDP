/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once
#include <cstdint>

namespace tools {
	/*
	* Holds transmitted bytes
	*/
	class Counters final
	{
	public:
		volatile size_t sent{ 0 };
		volatile size_t  received{ 0 };

		/* Resets counters to 0
		*/
		void clear() noexcept;

		/* Returns true if counters values are equal
		*/
		bool operator==(const Counters& other) const noexcept;

		/* Returns true if counters values are not equal
		*/
		bool operator!=(const Counters& other) const noexcept;
	};
}
