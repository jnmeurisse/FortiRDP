/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <iostream>
#include <algorithm>
#include <string>
#include <cctype>

#include "tools/StrUtil.h"
#include "http/Headers.h"

namespace http {

	Headers::Headers() :
		_headers()
	{
	}


	Headers::~Headers()
	{
		clear();
	}


	void Headers::clear()
	{
		_headers.clear();
	}


	Headers& Headers::add(const Headers& headers)
	{
		for (auto iter = headers._headers.cbegin(); iter != headers._headers.cend(); iter++) {
			set(iter->first, iter->second);
		}

		return *this;
	}


	Headers& Headers::set(const std::string& name, const std::string& value)
	{
		_headers.set(name, value);
		
		return *this;
	}

	
	Headers& Headers::set(const std::string& name, const int value)
	{
		return set(name, std::to_string(value));
	}


	Headers& Headers::set(const std::string& name, const size_t value)
	{
		return set(name, std::to_string(value));
	}


	bool Headers::get(const std::string& name, std::string& value) const
	{
		return _headers.get_str(name, value);
	}


	void Headers::write(tools::ByteBuffer& buffer) const
	{
		for (auto iter = _headers.cbegin(); iter != _headers.cend(); iter++) {
			buffer.append(iter->first).append(": ").append(iter->second).append("\r\n");
		}
	}

}
