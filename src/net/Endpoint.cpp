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
		Endpoint()
	{
		init(address, default_port);
	}


	std::string Endpoint::to_string() const
	{
		return _hostname + ":" + std::to_string(_port);
	}


	bool Endpoint::is_undef() const
	{
		return (_hostname.compare("0.0.0.0") == 0) && (_port == 0);
	}


	void Endpoint::init(const std::string& address, const int default_port)
	{
		bool valid_port = true;

		const char* const str = address.c_str();
		const char* p = address.c_str() + address.length() - 1;

		// search a port delimiter from the end of the string
		while (p >= str && *p != ':' && *p != ']')
			p--;

		if ((p >= str) &&  (*p == ':')) {
			// extract the host name and port
			_hostname = tools::trim(std::string(str, p));
			valid_port = tools::str2i((std::string(p + 1, str + address.length())), _port) &&
							(_port > 0);
		}
		else {
			_hostname = tools::trim(str);
			_port = default_port;
		}

		if (_hostname.length() == 0 || !valid_port) {
			throw std::invalid_argument("Invalid address syntax.");
		}
	}
}
