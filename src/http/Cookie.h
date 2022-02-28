/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include "http/CookieError.h"
#include "tools/ObfuscatedString.h"

namespace http {

	/**
	* Stores an HTTP Cookie.
	* (See rfc6265)
	*/
	class Cookie final
	{
	public:
		/* Allocates a cookie from a raw value. This constructor throws
		 * a CookieError exception if the cookie can not be decoded according
		 * to rfc6265.
		*/
		explicit Cookie(const tools::obfstring& value);

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

		/* Returns the cookie expires attribute
		*/
		inline const std::string& get_expires() const noexcept { return _expires; }

		/* Returns true if the cookie secure attribute defined
		*/
		inline const bool is_secure() const noexcept { return _secure; }

		/* Returns true if the cookie attribute httponly is defined
		*/
		inline const bool is_http_only() const noexcept { return _http_only; }

	private:
		// Parsed cookie 
		std::string _name;
		tools::obfstring _value;
		std::string _domain;
		std::string _path;
		std::string _expires;
		bool _secure = false;
		bool _http_only = false;

		/* Parse a cookie into attributes
		*/
		bool parse(const tools::obfstring& value);
	};

}