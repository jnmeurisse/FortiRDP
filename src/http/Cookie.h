/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <ctime>
#include <string>
#include "tools/ObfuscatedString.h"


namespace http {

	/**
	* Stores an HTTP Cookie.
	* (See rfc6265)
	*/
	class Cookie final
	{
	public:
		Cookie() = delete;


		/* Allocates a cookie from the cookie attributes.
		*/
		Cookie(
			const std::string& name,
			const tools::obfstring& value,
			const std::string& domain,
			const std::string& path,
			const std::time_t& expires,
			const bool secure,
			const bool http_only
		);


		/* Returns an obfuscated string representation of this cookie.
		 * The string format is compatible with Netscape specification.
		*/
		tools::obfstring to_header() const;

		/* Returns the cookie name 
		*/
		inline const std::string& get_name() const noexcept { return _name; }

		/* Returns the cookie value as an obfuscated string
		*/
		inline const tools::obfstring& get_value() const noexcept { return _value; }

		/* Returns the cookie domain attribute
		*/
		inline const std::string& get_domain() const noexcept { return _domain; }

		/* Returns the cookie path attribute
		*/
		inline const std::string& get_path() const noexcept{ return _path; }

		/* Returns the cookie expires attribute.
		*  The method returns -1 if the cookie is a session cookie.
		*/
		inline const std::time_t& get_expires() const noexcept { return _expires; }

		/* Returns true if the cookie secure attribute defined
		*/
		inline const bool is_secure() const noexcept { return _secure; }

		/* Returns true if the cookie attribute httponly is defined
		*/
		inline const bool is_http_only() const noexcept { return _http_only; }

		/* Returns true if the cookie is expired
		*/
		bool is_expired() const noexcept;

		/* Returns true if it is a session cookie
		*/
		bool is_session() const noexcept;

		/* Constructs a cookie from a Set-cookie header string.
		*  @param cookie_string The cookie definition.
		*  @return a Cookie.
		*
		*  The method throws a CookieError if the cookie is not valid.
		*/
		static Cookie parse(const tools::obfstring& cookie_string);

	private:
		// cookie attributes
		std::string _name;
		tools::obfstring _value;
		std::string _domain;
		std::string _path;
		std::time_t _expires;
		bool _secure;
		bool _http_only;


		/* Convert a Cookie time representation to a time_t.
		*
		* @param value The string to convert to a time_t.
		* @return The corresponding time or 0 if the string can not be parsed correctly.
		* This expires the cookie.
		*/
		static time_t parse_http_date(const std::string& value);
	};

}
