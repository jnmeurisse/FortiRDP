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
#include "util/ErrUtil.h"


namespace net {

	/**
	* DNSClient is a static class used to query the DNS server.  The DNS server address
	* is obtained from the PPP server during the connection (see pppossl_connect). 
	*/
	class DnsClient
	{
	public:
		DnsClient() = delete;
		~DnsClient() = delete;

		/**
		 * Returns true if a DNS server address is configured.
		*/
		static bool is_configured();

		/**
		 * Returns the DNS server address as a string.
		*/
		static std::string dns();

		/**
		 * Gets the IP address of a given host name.
		 *
		 * The function returns ERR_OK if the address is available in the DNS client 
		 * local cache and copies the address in addr.
		 *
		 * The function returns ERR_INPROGRESS when the DNS request is queued to
		 * be sent to the DNS server.  The found_callback is called later with the
		 * result of the DNS query.
		*/
		static utl::lwip_err query(const std::string& hostname, ip_addr_t& addr, dns_found_callback found_callback, void* callback_arg);
	};

}
