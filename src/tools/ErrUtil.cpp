/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <sstream>

#include "tools/ErrUtil.h"
#include "tools/StrUtil.h"

#include "lwip/def.h"

namespace tools {
	std::wstring win_errmsg(const win_err errnum)
	{
		wchar_t *buffer;
		std::wstring message;

		if (::FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			errnum,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&buffer,
			0,
			NULL) != 0)
		{
			message = buffer;
			LocalFree(buffer);
		}

		return message;
	}


	std::string ossl_errmsg(const ossl_err errnum)
	{
		std::ostringstream os;

		// get the OpenSSL error message (can be null if errnum is unknown)
		const char* errmsg = ::ERR_reason_error_string(errnum);
		if (!errmsg) {
			errmsg = "Unknown error";
		} 

		os << errmsg << " (ossl 0x" << std::hex << errnum << ")";

		return os.str();
	}


	std::string lwip_errmsg(const lwip_err errnum)
	{
		std::ostringstream os;

		// get the lwip error message (lwip_strerr always return an error message)
		const char* errmsg = ::lwip_strerr(errnum);

		// format the error message
		os << errmsg << " (lwip -0x" << std::hex << (int)-errnum << ")";

		return os.str();
	}


	std::string ppp_errmsg(const ppp_err errnum)
	{
		const char*errmsg[13] = {
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

		if ((errnum < 0) || (errnum >= LWIP_ARRAYSIZE(errmsg))) {
			os << "Unknown error " << " (" << errnum << ")";
		}
		else {
			// format the error message
			os << errmsg[errnum] << " (" << errnum << ")";
		}

		return os.str();
	}


	std::string ossl_error::message() const
	{
		return ossl_errmsg(_errnum);
	}
}
