/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include "lwip/ip_addr.h"

namespace net {

	/**
	 * An Endpoint identifies a network address. It is defined by the combination
	 * of a hostname and a port identifier.
	*/
	class Endpoint
	{
	public:
		/* Allocates a default endpoint mapped to 0.0.0.0:0
		*/
		Endpoint();

		/* Allocates an endpoint from an address
		 * @param address The endpoint address
		 * @param default_port The default port if not specified in the address.
		 *
		 * The address must respect the following syntax : hostname{:port}
		 * If not specified, port defaults to the value specified by the default_port
		 * when the endpoint was created. This method throws an invalid_argument exception
		 * if the syntax is not valid or if the port is not a valid integer.
		 *
		*/
		explicit Endpoint(const std::string& address, const int default_port);

		/* Copy constructor
		*/
		explicit Endpoint(const Endpoint& ep);

		/* Copy operator
		*/
		Endpoint& operator= (const Endpoint& other);

		/* Converts this Endpoint to a printable string
		*/
		std::string to_string() const;

		/* Returns the hostname of this end point
		*/
		inline const std::string& hostname() const { return _hostname; }

		/* Returns the port of this end point
		*/
		inline const int port() const { return _port; }

		/* Returns true if this end point is empty
		*/
		bool is_empty() const;

		/* Checks if this is a valid representation of an IP address
		*/
		bool is_ipaddr(ip4_addr_t& addr) const;

	private:
		std::string _hostname;
		int _port;

		/* Initializes this end point from the specified parameters
		 * @param address The endpoint address
		 * @param default_port The default port if not specified in the address.
		*/
		void init(const std::string& address, int default_port);
	};
}