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

	class PortForwarders final : public std::list<PortForwarder*>
	{
	public:
		/* Allocates an empty forwarder list.
		*/
		PortForwarders();

		/* Deletes all existing forwarders in that list.
		*/
		~PortForwarders();

		/* Deletes all port forwarders from the list having the given state.
		*  Returns the number of deleted forwarders.
		*/
		size_t delete_having_state(state_check_cb check_cb);

		/* Disconnects all port forwarders.
		*  Returns the number of disconnected forwarders.
		*/
		size_t abort_all();

		/* Returns true if at least one port forwarder is trying to connect.
		*/
		bool has_connecting_forwarders() const;

		/* Returns the number of connected forwarders
		*/
		size_t connected_count() const noexcept;

	private:
		// a reference to the application logger
		tools::Logger* const _logger;
	};
}