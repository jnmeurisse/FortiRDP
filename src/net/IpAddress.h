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
#include <optional>

namespace net {
	/**
	 * This class encapsulates a lwIP IP address and exposes helper functions
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
		 * Constructor.
		 * Initializes the address from a lwIP address.
		 */
		IpAddress(const ip_addr_t& address);

		/**
		 * Copy constructor.
		 * 
		 * @param address Address to copy from.
		 */
		IpAddress(const IpAddress& address);

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
		 * Returns the address as a raw ip_addr_t value.
		 */
		inline const ip_addr_t& get_address() const noexcept { return _ip_addr; }

		/**
		 * Checks whether the address is the any address (0.0.0.0).
		 * @return True if address is "any".
		 */
		bool is_any() const noexcept;

		/**
		 * Allocates an IpAddress from a string representation.
		 *
		 * @param address IPv4 string (e.g., "192.168.1.1").
		 * @return the address.
		 */
		static std::optional<IpAddress> from_string(const std::string& address);

		/**
		 * Converts the address to string form.
		 * 
		 * @return IP address string representation.
		 */
		std::string to_string() const noexcept;

	private:
		ip_addr_t _ip_addr;
	};

}