/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include <lwip/ip_addr.h>

namespace net {
	/**
	 * This class encapsulates a lwIP IPv4 address and exposes helper functions
	 * for parsing, formatting, comparison, and state checks.
	 */
	class IpAddress
	{
	public:
		/**
		 * Default constructor.
		 * Initializes the address to 0.0.0.0.
		 */
		IpAddress();

		/**
		 * Copy constructor.
		 * 
		 * @param ip_address Address to copy from.
		 */
		IpAddress(const IpAddress& ip_address);

		/**
		 * Assignment operator.
		 * 
		 * @param other Address to assign from.
		 * @return Reference to this object.
		 */
		IpAddress& operator=(const IpAddress& other);

		/**
		 * Equality comparison.
		 * 
		 * @param other Address to compare with.
		 * @return True if addresses are identical.
		 */
		bool operator==(const IpAddress& other) const;

		/**
		 * Inequality comparison.
		 * 
		 * @param other Address to compare with.
		 * @return True if addresses differ.
		 */
		bool operator!=(const IpAddress& other) const;

		/**
		 * @brief Resets the address to zero (0.0.0.0).
		 */
		void clear() noexcept;

		/**
		 * Sets the address from a string representation.
		 * 
		 * @param address IPv4 string (e.g., "192.168.1.1").
		 * @return true if the address could be converted, false on failure.
		 */
		bool set_address(const std::string& address);

		/**
		 * Sets the address from a raw ip_addr_t value.

		 * @param address Raw address to copy.
		 * @return Always true.
		 */
		bool set_address(const ip_addr_t& address);

		/**
		 * Returns the address as a raw ip_addr_t value.
		 */
		inline const ip_addr_t& get_address() const{ return _addr; }

		/**
		 * Checks whether the address is the any address (0.0.0.0).
		 * @return True if address is "any".
		 */
		bool is_any() const noexcept;

		/**
		 * Converts the address to string form.
		 * 
		 * @return Dotted IPv4 string representation.
		 */
		std::string to_string() const noexcept;

	private:
		ip_addr_t _addr;
	};

}