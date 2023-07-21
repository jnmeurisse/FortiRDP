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

	// openssl error codes
	using ossl_err = int;

	// lwip error codes
	using lwip_err = err_t;

	// ppp over fortigate sslvpn error codes
	using ppp_err = int;

	// Returns winapi error message 
	std::wstring win_errmsg(const win_err errnum);

	// Returns openssl error message
	std::string ossl_errmsg(const ossl_err errnum);

	// Returns lwip error message
	std::string lwip_errmsg(const lwip_err errnum);

	// Returns ppp error message
	std::string ppp_errmsg(const ppp_err errnum);


	class frdp_error : public std::exception {
	public:
		virtual std::string message() const noexcept = 0;
	};

	class ossl_error : public frdp_error {
	public:
		explicit ossl_error(ossl_err errnum) : _errnum(errnum >= 0 ? 0 : errnum) {};
		
		std::string message() const noexcept override;

	private:
		const ossl_err _errnum;
	};

	class httpcli_error : public frdp_error {
	public:
		httpcli_error(const std::string& message) : _message(message) {};

		std::string message() const noexcept override { return _message; }

	private:
		const std::string _message;
	};
}