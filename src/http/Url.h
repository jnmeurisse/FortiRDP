/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include "tools/StringMap.h"


namespace http {

	/**
	* A simplified URL dissector (based on RFC3986).
	*
	*/
	class Url final
	{
	public:
		/**
		 * Creates an empty URL.
		*/
		Url() = default;

		/**
		 * Creates an URL from the specified string.
		 * 
		 * Note: No validation of the input string is performed by this constructor.
		*/
		explicit Url(const std::string& url);

		/**
		 * Create an URL from the different parts (scheme, authority, path, ...)
		 * 
		 *  No validation of the parameters is performed by this constructor.
		*/
		Url(const std::string& scheme, const std::string& authority,
			const std::string& path, const std::string& query, const std::string& fragment = "");

		/**
		 * Copy constructor.
		*/
		Url(const Url& url) = default;

		/**
		 * Returns the scheme from this URL.
		*/
		inline const std::string& get_scheme() const noexcept { return _scheme; }

		/**
		 * Returns the authority from this URL.
		*/
		inline const std::string& get_authority() const noexcept { return _authority; }

		/*
		 * Returns the host name from the URL authority.
		*/
		std::string get_hostname() const;

		/**
		 * Returns the path from this URL.
		*/
		inline const std::string& get_path() const noexcept { return _path; }

		/**
		 * Returns the query part from this URL.
		*/
		inline const std::string& get_query() const noexcept { return _query; }

		/**
		 * Returns the fragment part from this URL.
		*/
		inline const std::string& get_fragment() const noexcept{ return _fragment; }

		/**
		 * Returns the query part as a string map.
		*/
		tools::StringMap get_query_map() const;
		
		/**
		 * Returns a string representation of this URL.
		 * 
		 * Note: when setting implicit argument to true, the returned
		 * url does not contain the scheme and authority.
		*/
		std::string to_string(bool implicit) const;

	private:
		// Components of an URL.
		std::string _scheme;
		std::string _authority;
		std::string _path;
		std::string _query;
		std::string _fragment;
	};

}
