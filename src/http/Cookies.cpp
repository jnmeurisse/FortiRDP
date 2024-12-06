/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Cookies.h"

#include <string>
#include "tools/StrUtil.h"


namespace http {

	Cookies::Cookies(const Cookies& cookies, const http::Url& url) :
		_cookies()
	{
		const std::string& valid_path = tools::trim(url.get_path());

		for (auto it = cookies.cbegin(); it != cookies.cend(); it++) {
			const Cookie& cookie{ it->second };
			const std::string path = tools::trim(cookie.get_path());

			if (path.size() == 0 || valid_path.compare(path) == 0) {
				// cookie without path or same path
				add(cookie);
			}
		}
	}


	void Cookies::clear()
	{
		_cookies.clear();
	}


	const Cookies& Cookies::add(const Cookies cookies)
	{
		for (auto it = cookies.cbegin(); it != cookies.cend(); it++) {
			add(it->second);
		}

		return *this;
	}


	const Cookies& Cookies::add(const Cookie& cookie)
	{
		// check if the name exists in this collection. 
		auto it = _cookies.find(cookie.get_name());

		if (it != _cookies.end()) {
			// If yes, update the value
			it->second = cookie;
		}
		else {
			// If no, insert the name-value pair
			_cookies.insert(std::pair<const std::string, Cookie>(cookie.get_name(), cookie));
		}

		return *this;
	}


	void Cookies::remove(const std::string& name)
	{
		_cookies.erase(name);
	}


	const Cookie& Cookies::get(const std::string& name) const
	{
		return _cookies.at(name);
	}


	bool Cookies::exists(const std::string& name) const
	{
		return _cookies.find(name) != _cookies.end();
	}


	tools::obfstring Cookies::to_header() const
	{
		tools::obfstring buffer;

		// Create the Set-Cookie header
		for (auto it = _cookies.cbegin(); it != _cookies.cend(); it++) {
			buffer.append(it->second.to_header());
			buffer.append("; ");
		}

		return buffer.size() > 0 ? buffer.substr(0, buffer.size() - 2) : tools::obfstring();
	}
}