/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "DnsClient.h"
#include <string>

namespace net {

	void DnsClient::set_server(uint8_t num, const net::IpAddress& server)
	{
		::dns_setserver(num, &server.address());
	}


	net::IpAddress DnsClient::get_server(uint8_t num)
	{
		net::IpAddress address;
		address.set_address(*dns_getserver(num));
		return address;
	}


	bool DnsClient::is_configured()
	{
		bool configured = false;
		for (uint8_t num = 0; num < DNS_MAX_SERVERS && !configured; num++)
			configured = !get_server(num).is_any();

		return configured;
	}


	std::string DnsClient::to_string()
	{
		std::string buffer;

		for (uint8_t num = 0; num < DNS_MAX_SERVERS; num++) {
			const IpAddress addr = get_server(num);
			if (!addr.is_any()) {
				buffer.append(addr.to_string()).append(",");
			}
		}

		return buffer.length() == 0 ? buffer : buffer.substr(0, buffer.length() - 1);
	}


	utl::lwip_err DnsClient::query(
		const std::string& hostname, ip_addr_t& addr, dns_found_callback found_callback, void* callback_arg)
	{
		return ::dns_gethostbyname_addrtype(
			hostname.c_str(),
			&addr,
			found_callback,
			callback_arg,
			LWIP_DNS_ADDRTYPE_IPV4
		);
	}

}
