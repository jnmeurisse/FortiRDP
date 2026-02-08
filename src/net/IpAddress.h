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

	class IpAddress {
	public:
		IpAddress();
		IpAddress(const IpAddress& addreip_addressss);
		IpAddress& operator=(const IpAddress& other);

		void clear() noexcept;

		bool set_address(const std::string& address);

		bool is_any() const noexcept;
		std::string to_string() const noexcept;

	private:
		ip_addr_t _addr;
	};

}