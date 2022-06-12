/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once
#include <string>
#include "lwip/dns.h"
#include "lwip/ip_addr.h"


class DnsClient
{
public:
	DnsClient() = delete;
	~DnsClient() = delete;

	static bool configured();

	static std::string dns();
	static err_t query(const std::string& hostname, ip_addr_t& addr, dns_found_callback found_callback, void* callback_arg);

private:
	/*
	*/
	static std::string dns(int numdns);


};

