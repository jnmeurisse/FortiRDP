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
	* A simplified URL dissector (based on RFC3986) 
	*/
	class Url final
	{
	public:
		Url() = default;

		Url(const Url& url);

		explicit Url(const std::string& scheme, const std::string& authority,
			const std::string& path, const std::string& query, const std::string& fragment="");

		/* Creates an URL from the specified string
		*/
		Url(const std::string& url);

		/* Returns the authority from this URL
		*/
		inline const std::string get_authority() const noexcept { return _authority; }

		/* Returns the path from this URL
		*/
		inline const std::string& get_path() const noexcept { return _path; }

		/* Returns the query part from this URL
		*/
		inline const std::string& get_query() const noexcept { return _query; }

		/* Returns the fragment part from this URL
		*/
		inline const std::string& get_fragment() const noexcept{ return _fragment; }

		/*
		*/
		tools::StringMap get_query_map() const;
		
		/*
		*/
		std::string to_string(bool implicit) const;

	private:
		// Components of an URL
		std::string _scheme;
		std::string _authority;
		std::string _path;
		std::string _query;
		std::string _fragment;
	};
}
