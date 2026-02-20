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
		ip_addr_set_zero(&_addr);
	}


	IpAddress::IpAddress(const IpAddress& ip_address)
	{
		set_address(ip_address._addr);
	}


	IpAddress& IpAddress::operator=(const IpAddress& other)
	{
		set_address(other._addr);
		return *this;
	}


	bool IpAddress::operator==(const IpAddress& other) const
	{
		return ip_addr_eq(&_addr, &other._addr);
	}


	bool IpAddress::operator!=(const IpAddress& other) const
	{
		return !ip_addr_eq(&_addr, &other._addr);
	}


	void IpAddress::clear() noexcept
	{
		ip_addr_set_zero(&_addr);
	}


	bool IpAddress::set_address(const std::string& address)
	{
		return ipaddr_aton(address.c_str(), &_addr) == 1;
	}


	bool IpAddress::set_address(const ip_addr_t& address)
	{
		ip_addr_copy(_addr, address);
		return true;
	}


	bool IpAddress::is_any() const noexcept
	{
		return ip_addr_isany_val(_addr);
	}


	std::string IpAddress::to_string() const noexcept
	{
		return ipaddr_ntoa(&_addr);
	}

}
