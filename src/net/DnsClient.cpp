/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <sstream>
#include "net/DnsClient.h"


bool DnsClient::configured()
{
	return !ip4_addr_isany_val(*dns_getserver(0)) || !ip4_addr_isany_val(*dns_getserver(1));
}


std::string DnsClient::dns()
{
	std::ostringstream os;
	const ip_addr_t *addr1 = dns_getserver(0);
	const ip_addr_t *addr2 = dns_getserver(1);

	if (!ip4_addr_isany_val(*addr1))
		os << ip4addr_ntoa(addr1);

	if (!ip4_addr_isany_val(*addr1) && !ip4_addr_cmp(addr1, addr2))
		os << ip4addr_ntoa(addr2);

	return os.str();
}


err_t DnsClient::query(const std::string& hostname, ip_addr_t& addr, dns_found_callback found_callback, void* callback_arg)
{
	return dns_gethostbyname_addrtype(hostname.c_str(), &addr, found_callback, callback_arg, LWIP_DNS_ADDRTYPE_IPV4);
}


std::string DnsClient::dns(int numdns)
{
	const ip_addr_t *addr = dns_getserver(numdns);
	return ip4_addr_isany_val(*addr) ? std::string() : ip4addr_ntoa(addr);
}
