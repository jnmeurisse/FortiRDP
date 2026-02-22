/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "IpAddress.h"
#include <lwip/ip_addr.h>

namespace net {

	IpAddress::IpAddress()
	{
		ip_addr_set_zero(&_ip_addr);
	}


	IpAddress::IpAddress(const ip_addr_t& address)
	{
		ip_addr_copy(_ip_addr, address);
	}


	IpAddress::IpAddress(const IpAddress& address)
	{
		ip_addr_copy(_ip_addr, address._ip_addr);
	}


	IpAddress& IpAddress::operator=(const IpAddress& other)
	{
		ip_addr_copy(_ip_addr, other._ip_addr);
		return *this;
	}


	bool IpAddress::operator==(const IpAddress& other) const
	{
		return ip_addr_eq(&_ip_addr, &other._ip_addr);
	}


	bool IpAddress::operator!=(const IpAddress& other) const
	{
		return !ip_addr_eq(&_ip_addr, &other._ip_addr);
	}


	void IpAddress::clear() noexcept
	{
		ip_addr_set_zero(&_ip_addr);
	}


	bool IpAddress::is_any() const noexcept
	{
		return ip_addr_isany_val(_ip_addr);
	}


	std::optional<IpAddress> from_string(const std::string& address)
	{
		ip_addr_t ip_address;
		if (ipaddr_aton(address.c_str(), &ip_address) == 1)
			return IpAddress(ip_address);

		return {};
	}


	std::string IpAddress::to_string() const noexcept
	{
		return ipaddr_ntoa(&_ip_addr);
	}

}
