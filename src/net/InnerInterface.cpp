/*!
* This file is part of FortiRDP
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "net/InnerInterface.h"
#include "util/Logger.h"

namespace net {
	using namespace utl;


	InnerInterface::InnerInterface(net::TlsSocket& tunnel, const net::IpAddress& address):
		_logger(Logger::get_logger()),
		_tunnel(tunnel),
		_counters(),
		_nif(),
		_output_queue(32 * 1024)
	{
		DEBUG_CTOR(_logger);
	}


	InnerInterface::~InnerInterface()
	{
		DEBUG_DTOR(_logger);
	}


	bool InnerInterface::send()
	{
		TRACE_ENTER(_logger);
		mbed_err rc = 0;

		if (!_output_queue.is_empty()) {
			size_t written = 0;
			rc = _output_queue.write(_tunnel, written);
			LOG_TRACE(_logger, "rc=%d sbytes=%zu", rc, written);

			if (rc == 0) {
				_counters.sent += written;
			}
			else {
				_logger->error("ERROR: %s - tunnel send failure (%d)", __class__, rc);
			}
		}

		LOG_TRACE(_logger, "socket fd=%d rc=%d", _tunnel.get_fd(), rc);

		return rc == 0;
	}



}
