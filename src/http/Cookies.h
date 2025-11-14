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
#include "http/Url.h"
#include "http/Cookie.h"
#include "tools/ObfuscatedString.h"


namespace http {

	/**
	* A collection of cookies.
	*/
	class Cookies final
	{
	public:
		using cookiemap = std::map<const std::string, Cookie> ;
		using const_iterator = cookiemap::const_iterator ;

		/**
		 * Allocates an empty cookies collection.
		*/
		Cookies() = default;

		/**
		 * Clears this cookies collection.
		*/
		void clear();

		/**
		 * Copies all cookies from the specified collection to this collection.
		 * 
		 * Note: Existing cookies could be replaced during this operation.
		 *
		 * @param cookies The collection of cookies to be copied into this collection.
		*/
		const Cookies& add(const Cookies cookies);

		/**
		 * Adds a cookie to the collection.
		 * 
		 * Note: If a cookie with the same name exists, it will be replaced 
		 * with the given cookie.
		 *
		 * @param cookie The cookie to add to the collection.
		*/
		const Cookies& add(const Cookie& cookie);

		/**
		 * Removes the cookie with the specified name from this collection.
		 *
		 * @param name The name of the cookie to be removed.
		 */
		void remove(const std::string& name);

		/**
		 * Gets the cookie with the specified name.
		 * 
		 * Note: The function throws an out_of_range exception if the cookie
		 * name does not exist in this collection.
		 *
		 * @param name The name of the cookie to be retrieved.
		 * @return A reference to the cookie.
		*/
		const Cookie& get(const std::string& name) const;

		/**
		* Returns true if the cookie with the specified name exits.
		*/
		bool exists(const std::string& name) const;

		/**
		 * Returns the number of cookies in this collection.
		*/
		inline size_t size() const { return _cookies.size(); }

		/**
		 * Constructs a string representation of the cookies collection.
		 * 
		 * Notes :
		 * The string format is compatible with Netscape specification and
		 * can be added to a Cookie header.  Only cookies that match the
		 * given url are added to the returned string.
		 *
		 * The implementation checks only if the URL domain and the cookie
		 * domain are the same.
		 *
		 * @param url The url used to select the cookies added to the returned string.
		*/
		tools::obfstring to_header(const Url &url) const;

		/**
		 * Returns an iterator referring to the first element in the collection.
		*/
		inline const_iterator cbegin() const { return _cookies.cbegin(); }

		/**
		 * Returns an iterator referring to the last element in the collection.
		*/
		inline const_iterator cend() const { return _cookies.cend(); }

	private:
		cookiemap _cookies;
	};

}
