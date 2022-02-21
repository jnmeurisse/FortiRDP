/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <list>
#include <functional>
#include "net/PortForwarder.h"
#include "tools/Logger.h"

namespace net {
	using state_check_cb = std::function<bool(PortForwarder *)>;

	class PortForwarders : public std::list<PortForwarder*>
	{
	public:
		/* Allocates an empty forwarder list.
		*/
		PortForwarders();

		/* Delete all existing forwarders in that list.
		*/
		~PortForwarders();

		/* Delete all port forwarders from the list in the given state.
		*  Returns the number of deleted forwarders.
		*/
		int delete_having_state(state_check_cb check_cb);

		/* Disconnects all port forwarders.
		*  Returns the number of disconnected forwarders.
		*/
		int abort_all();

		/* Returns true if at least one port forwarder is trying to connect.
		*/
		bool has_connecting_forwarders() const;

	private:
		// a reference to the application logger
		tools::Logger* const _logger;
	};
}