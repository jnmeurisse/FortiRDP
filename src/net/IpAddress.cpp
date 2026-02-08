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

	IpAddress::IpAddress() : 
		_addr(0)
	{
	}


	IpAddress::IpAddress(const IpAddress& ip_address)
	{
		ip_addr_copy(_addr, ip_address._addr);
	}


	IpAddress& IpAddress::operator=(const IpAddress& other)
	{
		ip_addr_copy(_addr, other._addr);
		return *this;
	}


	void IpAddress::clear() noexcept
	{
		ip_addr_set_zero(&_addr);
	}


	bool IpAddress::set_address(const std::string& address)
	{
		return ipaddr_aton(address.c_str(), &_addr) != 1;
	}


	bool IpAddress::is_any() const noexcept
	{
		return ip_addr_isany_val(_addr);
	}


	std::string IpAddress::to_string() const noexcept
	{
		return ip4addr_ntoa(&_addr);
	}

}
