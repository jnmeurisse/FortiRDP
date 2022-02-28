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
#include <exception>
#include "lwip/err.h"

namespace tools {
	// Windows error codes
	using win_err = DWORD;

	// mbed tls error codes
	using mbed_err = int;

	// lwip error codes
	using lwip_err = err_t;

	// ppp over fortigate sslvpn error codes
	using ppp_err = int;


	class tnl_error : public std::exception {
	public:
		explicit tnl_error(int errnum) : _errnum(errnum) {};
		
		inline int errnum() const noexcept { return _errnum; }
		virtual std::string message() const = 0;

	protected:
		const int _errnum;
	};

	
	class mbed_error final : public tnl_error {
	public:
		explicit mbed_error(int errnum): tnl_error(errnum >= 0 ? 0 : errnum) {};
		
		std::string message() const override;
	};


	class lwip_error final : public tnl_error {
	public:
		explicit lwip_error(int errnum) : tnl_error(errnum) {};
		
		std::string message() const override;
	};


	class http_error final : public tnl_error {
	public:
		static const int CHUNK_SIZE = -1;
		static const int BODY_SIZE = -2;

		explicit http_error(int errnum) : tnl_error(errnum) {};
		
		std::string message() const override;
	};


	// Returns winapi error message 
	std::wstring win_errmsg(const win_err errnum);

	// Returns mbedtls error message
	std::string mbed_errmsg(const mbed_err errnum);

	// Returns lwip error message
	std::string lwip_errmsg(const lwip_err errnum);

	// Returns ppp error message
	std::string ppp_errmsg(const ppp_err errnum);
}