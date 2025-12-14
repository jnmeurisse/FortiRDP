/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "StringMap.h"

#include <vector>


namespace tools {

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
		if (_strmap.size() > 0) {
			strimap::iterator iter;

			for (iter = _strmap.begin(); iter != _strmap.end(); iter++) {
				tools::serase(iter->second);
			}
		}

		_strmap.clear();
	}


	size_t StringMap::add(const std::string& line, char delim)
	{
		std::vector<std::string> tokens;

		if (tools::split(line.c_str(), delim, tokens)) {
			for (size_t idx = 0; idx < tokens.size(); idx++) {
				const std::string item{ tokens.at(idx) };

				// Skip empty item.
				if (item.size() == 0)
					continue;

				// Allow item without value.
				size_t pos = item.find('=');
				if (pos != std::string::npos) {
					// The spaces before the value are not significant
					set(trim(item.substr(0, pos)), trimleft(item.substr(pos + 1, std::string::npos)));

				}
				else {
					set(trim(item), "");
				}
			}
		}

		return tokens.size();
	}


	void StringMap::set(const std::string& name, const std::string& value)
	{
		tools::strimap::iterator it;

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
		tools::strimap::const_iterator it{ _strmap.find(name) };

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

		return get_str(name, tmp) && tools::str2i(tmp, value);
	}


	int StringMap::get_int_value(const std::string& name, int default_value) const
	{
		int value;
		return get_int(name, value) ? value : default_value;
	}


	std::string StringMap::join(const std::string& delim) const
	{
		std::string buffer;

		if (_strmap.size() > 0) {
			StringMap::const_iterator iter;

			for (iter = cbegin(); iter != cend(); iter++) {
				buffer.append(iter->first).append("=").append(iter->second).append(delim);
			}

			buffer = buffer.substr(0, buffer.size() - delim.size());
		}

		return buffer;
	}

}
