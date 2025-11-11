/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Cookie.h"

#include <chrono>
#include <vector>

#include "http/CookieError.h"
#include "tools/StrUtil.h"
#include "tools/strptime.h"


namespace http {

	constexpr std::time_t EXPIRES_UNSPECIFIED = -1;


	static std::string normalize_domain(const std::string& domain)
	{
		return tools::trim(tools::lower(domain));
	}


	static std::string normalize_path(std::string path)
	{
		path = tools::trim(path);
		return path.empty() ? "/" : path;
	}


	Cookie::Cookie(
		const std::string& name,
		const tools::obfstring& value,
		const std::string& domain,
		const std::string& path,
		const std::time_t& expires,
		const bool secure,
		const bool http_only) :
		_name(name),
		_value(value),
		_domain(normalize_domain(domain)),
		_path(normalize_path(path)),
		_expires(expires < 0 ? EXPIRES_UNSPECIFIED : expires),
		_secure(secure),
		_http_only(http_only)
	{
	}


	tools::obfstring Cookie::to_header() const
	{
		return tools::obfstring{ _name + "=" }.append(_value);
	}


	bool Cookie::is_expired() const noexcept
	{
		using namespace std::chrono;

		// a session cookie expires at the end of session.
		if (is_session())
			return false;

		return system_clock::to_time_t(system_clock::now()) > _expires;
	}


	bool Cookie::is_session() const noexcept
	{
		return _expires == EXPIRES_UNSPECIFIED;
	}


	bool Cookie::same_domain(const std::string& domain) const
	{
		return _domain.compare(normalize_domain(domain)) == 0;
	}


	bool Cookie::path_matches(const std::string& path) const
	{
		std::string tmp = normalize_path(path);

		if (_path.compare(tmp) == 0)
			// same path
			return true;
		else if (tmp.compare(0, _path.size(), _path) == 0)
			// path starts with cookie path
			return true;
		else
			return false;
	}

	
	Cookie Cookie::parse(const tools::obfstring& cookie_string)
	{
		std::vector<tools::obfstring> parts;

		// Split the cookie string. Each cookie header must contain at least one cookie-pair.
		// A cookie-pair defines a cookie’s name and value according to the standard syntax:
		//    cookie-pair *( ";" SP cookie-av )
		//    cookie-pair = cookie-name "=" cookie-value
		// Additional attributes (cookie-av) are separated by semicolons.
		if (tools::split(cookie_string, ';', parts) <= 0)
			throw CookieError{ "Empty cookie definition" };
		
		// split cookie-pair 
		const tools::obfstring cookie_pair(parts[0]);
		const std::string::size_type pos = cookie_pair.find('=');
		if (pos == std::string::npos)
			throw CookieError{ "Invalid cookie :" + cookie_string.uncrypt() };

		const std::string cookie_name{ cookie_pair.substr(0, pos).uncrypt() };
		const tools::obfstring cookie_value{ cookie_pair.substr(pos + 1, std::string::npos) };
		std::string domain;
		std::string path;
		std::time_t expires = EXPIRES_UNSPECIFIED;
		bool secure = false;
		bool http_only = false;

		if (parts.size() > 1) {
			for (size_t idx = 1; idx < parts.size(); idx++) {
				// get next attribute-value
				const std::string cookie_av{ tools::trimleft(parts[idx].uncrypt()) };
				if (cookie_av.size() > 0) {
					// extract attribute name and value
					const std::string::size_type pos = cookie_av.find('=');

					if (pos != std::string::npos) {
						const std::string attribute_name{ cookie_av.substr(0, pos) };
						const std::string attribute_value{ cookie_av.substr(pos + 1, std::string::npos) };

						if (tools::iequal(attribute_name, "domain")) {
							domain = attribute_value;
						}
						else if (tools::iequal(attribute_name, "path")) {
							path = attribute_value;
						}
						else if (tools::iequal(attribute_name, "expires")) {
							expires = parse_http_date(attribute_value);
						}
						else if (tools::iequal(attribute_name, "max-age")) {
							// not supported, attribute is ignored
						}
					}
					else if (tools::iequal(cookie_av, "secure")) {
						secure = true;
					}
					else if (tools::iequal(cookie_av, "httponly")) {
						http_only = true;
					}
				}
			}
		}

		return Cookie(
			cookie_name,
			cookie_value,
			domain,
			path,
			expires,
			secure,
			http_only
		);
	}

	time_t Cookie::parse_http_date(const std::string& value)
	{
		static char* COOKIE_DATE_FORMATS[] =
		{
			"%a, %d %b %Y %H:%M:%S %Z",
			"%a, %d-%b-%Y %H:%M:%S %Z"
			"%a, %d %b %y %H:%M:%S %Z",
			"%a, %d-%b-%y %H:%M:%S %Z"
		};

		struct tm tm;

		for (unsigned int i = 0; i < std::size(COOKIE_DATE_FORMATS); i++) {
			tm = { 0 };

			char* const p = strptime(value.c_str(), COOKIE_DATE_FORMATS[i], &tm);
			if (p && tm.tm_year > 0)
				return mktime(&tm);
		}

		return 0;
	}

}
