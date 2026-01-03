/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "ErrUtil.h"

#include <array>
#include <sstream>
#include <mbedtls/error.h>
#include <lwip/err.h>


namespace utl {

	std::wstring win_errmsg(const win_err errnum)
	{
		wchar_t *buffer;
		std::wstring message;

		if (::FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			errnum,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPWSTR>(&buffer),
			0,
			nullptr) != 0)
		{
			message = buffer;
			LocalFree(buffer);
		}

		return message;
	}


	std::string mbed_errmsg(const mbed_err errnum)
	{
		std::ostringstream os;
		std::array<char, 2048> buffer = { 0 };

		// get mbedtls error
		::mbedtls_strerror(errnum, buffer.data(), buffer.size() - 1);

		// format the error message
		os << buffer.data() << " (-0x" << std::hex << -errnum << ")";

		return os.str();
	}


	std::string lwip_errmsg(const lwip_err errnum)
	{
		std::ostringstream os;
		const char* errmsg;

		// get the lwip error message
		errmsg = ::lwip_strerr(errnum);

		// format the error message
		os << errmsg << " (-0x" << std::hex << -errnum << ")";

		return os.str();
	}


	std::string ppp_errmsg(const ppp_err errnum)
	{
		static const std::array<std::string, 13> errmsg = {
			/* PERR_NONE    */			"",
			/* PPPERR_PARAM */			"Invalid parameter",
			/* PPPERR_OPEN  */			"Unable to open PPP session",
			/* PPPERR_DEVICE */			"Invalid I/O device for PPP",
			/* PPPERR_ALLOC */			"Unable to allocate resources",
			/* PPPERR_USER */			"User interrupt",
			/* PPPERR_CONNECT */		"Connection lost",
			/* PPPERR_AUTHFAIL */		"Failed authentication challenge",
			/* PPPERR_PROTOCOL */		"Failed to meet protocol",
			/* PPPERR_PEERDEAD */		"Connection timeout",
			/* PPPERR_IDLETIMEOUT */	"Idle timeout",
			/* PPPERR_CONNECTTIME */	"Max connect time reached",
			/* PPPERR_LOOPBACK */		"Loopback detected"
		};

		std::ostringstream os;

		if ((errnum < 0) || (errnum >= errmsg.size())) {
			os << "Unknown error.";
		}
		else {
			// Format the error message.
			os << errmsg[errnum] << " (" << errnum << ")";
		}

		return os.str();
	}


	mbed_error::mbed_error(mbed_err errnum) :
		frdp_error(mbed_errmsg(errnum)),
		_errnum(errnum >= 0 ? 0 : errnum)
	{
	}


	win_error::win_error(win_err error_code, const std::string& func_name) :
		std::system_error((int)error_code, std::system_category(), func_name)
	{
	}

}
