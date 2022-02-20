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
#include "tools/StrUtil.h"

namespace tools {

	/**
	* A case insensitive string to string map
	*/
	class StringMap
	{
	public:
		using const_iterator = strimap::const_iterator;

		/* Allocates an empty string map
		*/
		StringMap();

		/* Allocates a string map from a named value pairs list.
		 * The named value pairs list must have the following syntax
		 * NAME{=VALUE} *(delim NAME{=VALUE})
		*/
		explicit StringMap(const std::string& line, const char delim);

		/* Clears the string map
		*/
		void clear();

		/* Adds a collection of named value pairs initialized from a named value pairs list.
		 * The named value pairs list must have the following syntax
		 * NAME{=VALUE} *(delim NAME{=VALUE})
		 *
		 * @param line
		 * @param delim
		 * \return the number of pair name, value added to this collection
		*/
		size_t add(const std::string& line, const char delim);

		/* Sets the value mapped to the specified name
		*/
		void set(const std::string& name, const std::string& value);

		/* Gets the value mapped to the specified name. The function returns true
		* if the name exists in the collection. The parameter value is updated only
		* if the name has been found.
		*/
		bool get_str(const std::string& name, std::string& value) const;

		/* Gets the value mapped to the specified name. The function converts the
		 * value to an integer. The function returns true if the name exists in the collection
		 * and if the value can be converted to a integer. The parameter value is updated only
		 * if the name has been found.
		*/
		bool get_int(const std::string& name, int& value) const;

		/* Returns all named value pairs. The function returns a string that has the following
		 * format : NAME=VALUEdelimNAME=VALUE etc..
		*/
		std::string join(const std::string& delim) const;

		/* Returns the number of strings in the collection
		*/
		inline size_t size() const { return _strmap.size(); }

		/* Returns an iterator referring to the first element in the collection
		*/
		inline const_iterator cbegin() const { return _strmap.cbegin(); }

		/* Returns an iterator referring to the last element in the collection
		*/
		inline const_iterator cend() const { return _strmap.cend(); }
		
	private:
		strimap _strmap;
	};

}