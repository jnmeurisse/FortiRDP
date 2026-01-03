/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "PortForwarders.h"


namespace net {
	using namespace utl;


	PortForwarders::PortForwarders() :
		_logger(Logger::get_logger())
	{
		DEBUG_CTOR(_logger);
	}


	PortForwarders::~PortForwarders()
	{
		DEBUG_DTOR(_logger);

		delete_having_state([](const PortForwarder*) {return true; });
	}


	size_t PortForwarders::delete_having_state(const state_check_cb& check_cb)
	{
		size_t count = 0;

		for (auto it = begin(); it != end();) {
			PortForwarder* const pf = (*it);

			if (check_cb(pf)) {
				delete pf;
				it = erase(it);
				count++;
			}
			else {
				++it;
			}
		}

		return count;
	}


	size_t PortForwarders::abort_all() const
	{
		size_t counter = 0;
		for (auto* pf : *this) {
			if (pf && pf->connected()) {
				pf->abort();
				counter++;
			}
		}

		return counter;
	}


	bool PortForwarders::has_connecting_forwarders() const
	{
		for (const auto* pf : *this) {
			if (pf && pf->connecting()) {
				return true;
			}
		}

		return false;
	}


	size_t PortForwarders::connected_count() const noexcept
	{
		size_t counter = 0;
		for (const auto* pf : *this) {
			if (pf && pf->connected()) {
				counter++;
			}
		}

		return counter;
	}

	const char* PortForwarders::__class__ = "PortForwarders";
}
