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

	void Cookies::clear()
	{
		_cookies.clear();
	}


	const Cookies& Cookies::add(const Cookies& cookies)
	{
		for (auto it = cookies.cbegin(); it != cookies.cend(); it++) {
			add(it->second);
		}

		return *this;
	}


	const Cookies& Cookies::add(const Cookie& cookie)
	{
		// Check if the name exists in this collection. 
		auto it = _cookies.find(cookie.get_name());

		if (it != _cookies.end()) {
			// If yes, update the value
			it->second = cookie;
		}
		else {
			// If no, insert the name-value pair
			_cookies.insert(std::pair<std::string, Cookie>(cookie.get_name(), cookie));
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


	tools::obfstring Cookies::to_header(const Url& url) const
	{
		tools::obfstring buffer;
		const std::string url_domain = url.get_hostname();
		const bool secure_link = tools::iequal(url.get_scheme(), "https");

		// Create the Set-Cookie header
		for (auto it = _cookies.cbegin(); it != _cookies.cend(); it++) {
			const Cookie& cookie = it->second;
			// Filter cookies:
			//   - skip expired cookies
			//   - skip non-http cookies
			//   - skip non-secure cookies if the url scheme is https
			//   - apply path and domain matches
			if (!cookie.is_expired() &&
				cookie.is_http_only() &&
				(!secure_link || cookie.is_secure()) &&
				cookie.same_domain(url_domain) &&
				cookie.path_matches(url.get_path()))
			{
				buffer.append(cookie.to_header());
				buffer.append("; ");
			}
		}

		return buffer.size() > 0 ? buffer.substr(0, buffer.size() - 2) : tools::obfstring();
	}

}
