/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Headers.h"

#include <string>


namespace http {

	using namespace tools;

	Headers& Headers::add(const Headers& headers)
	{
		for (auto iter = headers.cbegin(); iter != headers.cend(); iter++) {
			StringMap::set(iter->first, iter->second);
		}

		return *this;
	}


	Headers& Headers::set(const std::string& name, const std::string& value)
	{
		StringMap::set(name, value);
		
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
		return StringMap::get_str(name, value);
	}


	void Headers::write(tools::ByteBuffer& buffer) const
	{
		for (auto iter = cbegin(); iter != cend(); iter++) {
			buffer.append(iter->first).append(": ").append(iter->second).append("\r\n");
		}
	}

}
