/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "StringMap.h"

#include <vector>
#include <utility>
#include "StrUtil.h"


namespace utl {

	StringMap::StringMap(const std::string& line, const char delim) :
		_strmap()
	{
		add(line, delim);
	}


	StringMap::~StringMap()
	{
		serase();
	}


	void StringMap::serase()
	{
		for (auto iter = _strmap.begin(); iter != _strmap.end(); ++iter) {
			str::serase(iter->second);
		}

		_strmap.clear();
	}


	size_t StringMap::add(const std::string& line, char delim)
	{
		std::vector<std::string> tokens;

		if (str::split(line.c_str(), delim, tokens) > 0) {
			for (const auto& item : tokens) {
				// Skip empty item.
				if (item.empty())
					continue;

				// Allow item without value.
				const size_t pos = item.find('=');
				if (pos != std::string::npos) {
					// The spaces after the equal sign are not significant
					set(str::trim(item.substr(0, pos)), str::trimleft(item.substr(pos + 1, std::string::npos)));
				}
				else {
					set(str::trim(item), "");
				}
			}
		}

		return tokens.size();
	}


	void StringMap::set(const std::string& name, const std::string& value)
	{
		strimap::iterator it;

		// Check if the name is already defined. If yes, update the 
		// associated value.
		it = _strmap.find(name);
		if (it != _strmap.end()) {
			it->second = value;
		}
		else {
			_strmap.insert(std::pair<const std::string, std::string>(name, value));
		}
	}


	bool StringMap::get_str(const std::string& name, std::string& value) const
	{
		bool found = false;
		strimap::const_iterator it{ _strmap.find(name) };

		if (it != _strmap.end()) {
			value = it->second;
			found = true;
		}

		return found;
	}


	std::string StringMap::get_str_value(const std::string& name, const std::string& default_value) const
	{
		std::string value;
		return get_str(name, value) ? value : default_value;
	}


	bool StringMap::get_int(const std::string& name, int& value) const
	{
		std::string tmp;
		return get_str(name, tmp) && str::str2i(tmp, value);
	}


	int StringMap::get_int_value(const std::string& name, int default_value) const
	{
		int value;
		return get_int(name, value) ? value : default_value;
	}


	std::string StringMap::join(const std::string& delim) const
	{
		std::string buffer;

		if (_strmap.empty())
			return buffer;

		buffer.reserve(256);
		for (auto iter = cbegin(); iter != cend(); ++iter) {
			buffer.append(iter->first).append("=").append(iter->second).append(delim);
		}

		return buffer.substr(0, buffer.size() - delim.size());
	}

}
