/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "net/PortForwarders.h"

namespace net {
	PortForwarders::PortForwarders() :
		_logger(Logger::get_logger())
	{
		DEBUG_CTOR(_logger, "PortForwarders");
	}


	PortForwarders::~PortForwarders()
	{
		DEBUG_DTOR(_logger, "PortForwarders");

		delete_having_state([](PortForwarder* pf) {return true; });
	}


	size_t PortForwarders::delete_having_state(state_check_cb check_cb)
	{
		size_t count = 0;

		for (auto it = begin(); it != end();) {
			PortForwarder* const pf = (*it);

			if (check_cb(pf)) {
				// delete this port forwarder
				delete pf;

				// move to next in the list
				it = erase(it);

				// One more deleted
				count++;
			}
			else {
				++it;
			}
		}

		return count;
	}


	size_t PortForwarders::abort_all()
	{
		size_t count = 0;

		for (auto it = begin(); it != end(); it++) {
			PortForwarder* const pf = (*it);

			if (pf->connected()) {
				pf->abort();

				count++;
			}
		}

		return count;
	}


	bool PortForwarders::has_connecting_forwarders() const
	{
		bool found = false;

		for (auto it = cbegin(); !found && it != end(); it++) {
			const PortForwarder* const pf = (*it);

			found = pf->connecting();
		}

		return found;
	}
}