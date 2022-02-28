/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <stdexcept>
#include "net/Endpoint.h"
#include "tools/StrUtil.h"

namespace net {

	Endpoint::Endpoint() :
		_hostname("0.0.0.0"),
		_port(0)
	{
	}


	Endpoint::Endpoint(const std::string& address, const int default_port) :
		_hostname("0.0.0.0"),
		_port(0)
	{
		init(address, default_port);
	}


	std::string Endpoint::to_string() const
	{
		return _hostname + ":" + std::to_string(_port);
	}


	bool Endpoint::is_empty() const
	{
		return _hostname.compare("0.0.0.0") == 0 && (_port == 0);
	}


	bool Endpoint::is_ipaddr(ip4_addr_t& addr) const
	{
		return ip4addr_aton(_hostname.c_str(), &addr) == 1;
	}


	void Endpoint::init(const std::string& address, const int default_port)
	{
		std::vector<std::string> tokens;
		bool valid_port = true;

		switch (tools::split(address.c_str(), ':', tokens))
		{
		case 1:
			_hostname = tools::trim(tokens[0]);
			_port = default_port;
			break;

		case 2:
			_hostname = tools::trim(tokens[0]);
			if (tokens[1].length() == 0) {
				_port = default_port;
			} else {
				valid_port = tools::str2i(tools::trim(tokens[1]), _port);
			}
			break;

		default:
			throw std::invalid_argument("Invalid address syntax.");
		}

		
		valid_port = valid_port && ((_port > 0 ) ||  (default_port == _port));

		if (_hostname.length() == 0 || !valid_port) {
			throw std::invalid_argument("Invalid address syntax.");
		}
	}

}