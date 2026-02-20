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
#include "net/IpAddress.h"
#include "util/ErrUtil.h"


namespace net {

	/**
	* DNSClient is a static class used to query the DNS servers.
	* 
	* The DNS server addresses are obtained from the PPP server during the
	* connection and configured in pppossl_connect.  Alternatively, the 
	* addresses can be set using the set_server method.
	*/
	class DnsClient
	{
	public:
		DnsClient() = delete;
		~DnsClient() = delete;

		/**
		 * Initialize one of the DNS servers.
		 *
		 * @param num the index of the DNS server to set, must be < DNS_MAX_SERVERS.
		 * @param server IP address of the DNS server to set
		 */
		static void set_server(uint8_t num, const net::IpAddress& server);

		/**
		 * Return one of the currently configured DNS server. 
		 * 
		 * @param num the index of the DNS server to get, must be < DNS_MAX_SERVERS.
		 * @return IP address of the indexed DNS server or "ip_addr_any" if the DNS
		 *         server has not been configured.
		 */
		static net::IpAddress get_server(uint8_t num);

		/**
		 * Returns true if a DNS server address is configured.
		*/
		static bool is_configured();

		/**
		 * Returns the DNS server address as a string.
		*/
		static std::string to_string();

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
