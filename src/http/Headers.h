/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include "tools/ByteBuffer.h"
#include "tools/StringMap.h"


namespace http {

	/** 
	* A collection of HTTP headers
	*/
	class Headers final : tools::StringMap
	{
	public:
		using tools::StringMap::serase;

		/**
		 * Copies all headers from the specified collection to this collection.
		 * 
		 * Note: Existing headers could be replaced during this operation.
		 *
		 * @param headers The collection of headers to be copied in this collection.
		*/
		Headers& add(const Headers& headers);

		/**
		 * Sets a header.
		 *
		 * @param name The name of the header.
		 * @param value The value of the header.
		*/
		Headers& set(const std::string& name, const std::string& value);

		/**
		 * Sets a header.
		 *
		 * @param name The name of the header.
		 * @param value The value of the header.
		*/
		Headers& set(const std::string& name, const int value);

		/**
		 * Sets a header
		 *
		 * @param name The name of the header.
		 * @param value The value of the header.
		*/
		Headers& set(const std::string& name, const size_t value);

		/**
		 * Gets a header with the specified name.
		 *
		 * @param name The name of the header.
		 * @param value The value of the header.
		*/
		bool get(const std::string& name, std::string& value) const;

		/**
		 * Writes all headers to the specified buffer
		 * 
		 * @param buffer The destination buffer.
		*/
		void write(tools::ByteBuffer& buffer) const;
	};

}
