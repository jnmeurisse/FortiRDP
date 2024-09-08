/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include <map>
#include "tools/ByteBuffer.h"
#include "tools/ObfuscatedString.h"
#include "http/Url.h"
#include "http/Cookie.h"

namespace http {

	/**
	* A collection of cookies.
	*/
	class Cookies final
	{
	public:
		using cookiemap = std::map<const std::string, Cookie> ;
		using const_iterator = cookiemap::const_iterator ;

		/* Allocates an empty cookies collection
		*/
		Cookies() = default;

		/* Clones a cookies collection. Only copy cookies associated with a given url. 
		*/
		explicit Cookies(const Cookies& cookies, const http::Url& url);

		/* Clears this cookies collection
		*/
		void clear();

		/* Copies all cookies from the specified collection to this collection.
		 * Existing cookies could be replaced during this operation.
		 *
		 * @param cookies The collection of cookies to be copied in this collection
		*/
		const Cookies& add(const Cookies cookies);

		/* Removes the cookie with the specified name from this collection
		 *
		 * @param name The name of the cookie
		 */
		void remove(const std::string& name);

		/* Sets the cookie with the specified name
		 *
		 * @param name The name of the cookie
		 * @param value The value of the cookie
		*/
		const Cookies& set(const std::string& name, const Cookie& value);

		/* Gets the cookie with the specified name. The function throws
		 * an out_of_range exception if the cookie name does not exist.
		 *
		 * @param name The name of the cookie to be retrieved
		 * @return A reference to the cookie
		*/
		const Cookie& get(const std::string& name) const;

		/* Returns true if the cookie with the specified name exits.
		*/
		bool exists(const std::string& name) const;

		/* Returns the number of cookies in this collection
		*/
		inline size_t size() const { return _cookies.size(); }

		/* Constructs a string representation of the cookies collection.
		 * The string format is compatible with Netscape specification and
		 * can be added to a Cookie header
		*/
		tools::obfstring to_header() const;

		/* Returns an iterator referring to the first element in the collection
		*/
		inline const_iterator cbegin() const { return _cookies.cbegin(); }

		/* Returns an iterator referring to the last element in the collection
		*/
		inline const_iterator cend() const { return _cookies.cend(); }

	private:
		cookiemap _cookies;
	};

}