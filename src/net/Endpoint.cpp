/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Endpoint.h"

#include <limits>
#include <stdexcept>
#include "tools/StrUtil.h"


namespace net {

	Endpoint::Endpoint() :
		_hostname("0.0.0.0"),
		_port(0)
	{
	}


	Endpoint::Endpoint(const std::string& address, const uint16_t default_port) :
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


	void Endpoint::init(const std::string& address, const uint16_t default_port)
	{
		bool valid_port = true;

		const char* const str = address.c_str();
		const char* p = address.c_str() + address.length() - 1;

		// Search a port delimiter from the end of the string.
		while (p >= str && *p != ':' && *p != ']')
			p--;

		if ((p >= str) &&  (*p == ':')) {
			// Extract the host name and port.
			_hostname = tools::trim(std::string(str, p));
			int port_value;
			valid_port = tools::str2i((std::string(p + 1, str + address.length())), port_value) 
						&& (port_value > 0) 
						&& (port_value <= std::numeric_limits<uint16_t>::max());
			if (valid_port)
				_port = static_cast<uint16_t>(valid_port);
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
