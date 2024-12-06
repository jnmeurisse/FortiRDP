/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include <lwip/dns.h>
#include <lwip/ip_addr.h>
#include <lwip/err.h>


namespace net {

	/**
	* DNSClient is a static class used to query the DNS server.  The DNS server address
	* is obtained from the ppp server during the connection (see pppossl_connect). 
	*/
	class DnsClient
	{
	public:
		DnsClient() = delete;
		~DnsClient() = delete;

		/* Returns true if a DNS server address is available.
		*/
		static bool configured();

		/* Returns the DNS server address as a string 
		*/
		static std::string dns();

		/* Gets the IP address of a given host name.
		*
		*  The function returns ERR_OK if the address is available in the dns client 
		*  local cache and copies the address in addr.
		*
		*  The function returns ERR_INPROGRESS when the dns request is queued to
		*  be sent to the dns server.  The found_callback is called later with the
		*  result of the dns query.
		*/
		static err_t query(const std::string& hostname, ip_addr_t& addr, dns_found_callback found_callback, void* callback_arg);
	};
}
