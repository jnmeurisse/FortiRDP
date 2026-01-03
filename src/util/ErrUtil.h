/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <Windows.h>
#include <string>
#include <stdexcept>
#include <lwip/err.h>


namespace aux {

	// Windows error codes
	using win_err = DWORD;

	// mbed tls error codes
	using mbed_err = int;

	// lwip error codes
	using lwip_err = err_t;

	// ppp over fortigate sslvpn error codes
	using ppp_err = int;

	// Returns winapi error message 
	std::wstring win_errmsg(const win_err errnum);

	// Returns mbedtls error message
	std::string mbed_errmsg(const mbed_err errnum);

	// Returns lwip error message
	std::string lwip_errmsg(const lwip_err errnum);

	// Returns ppp error message
	std::string ppp_errmsg(const ppp_err errnum);


	class frdp_error : public std::runtime_error {
	public:
		using std::runtime_error::runtime_error;
	};

	class mbed_error : public frdp_error {
	public:
		explicit mbed_error(mbed_err errnum);
		
	private:
		const mbed_err _errnum;
	};

}
