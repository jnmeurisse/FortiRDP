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
	* A simplified URL dissector
	*/
	class Url
	{
	public:
		/* Creates an URL from the specified value.
		*/
		explicit Url(const std::string& url_path);

		/* Returns the path (in fact everything before the query) from an URL
		*/
		inline const std::string& get_path() const { return _path; }

		/* Returns the query string from this URL
		*/
		inline const tools::StringMap& get_query() const { return _query; }

	private:
		// Components of an URL
		std::string _path;
		tools::StringMap _query;
	};

}