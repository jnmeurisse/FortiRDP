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
		volatile uint64_t sent;
		volatile uint64_t received;

		/* Default constructor
		*/
		Counters();

		/* Resets counters to 0
		*/
		void clear() noexcept;

		/* Assign
		*/
		void operator=(const Counters& other) noexcept;

		/* Returns true if counters values are equal
		*/
		bool operator==(const Counters& other) const noexcept;

		/* Returns true if counters values are not equal
		*/
		bool operator!=(const Counters& other) const noexcept;
	};
}
