/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once


namespace utl {

	/**
	* Holds transmitted bytes counters.
	*/
	class Counters final
	{
	public:
		volatile size_t sent{ 0 };
		volatile size_t received{ 0 };

		/**
		 * Resets counters to 0.
		*/
		void clear() noexcept;

		/**
		 * Compares this counters with other counters for equality.
		 * 
		 * @return true if counters values are equal.
		*/
		bool operator==(const Counters& other) const noexcept;

		/**
		 * Compares this counters with other counters for inequality.
		 *
		 * @return true if counters values are not equal.
		*/
		bool operator!=(const Counters& other) const noexcept;

		/**
		 * Returns the transmitted total.
		*/
		size_t total() const noexcept { return sent + received; }
	};

}
