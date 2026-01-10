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
#include "util/Logger.h"


namespace net {

	using state_check_cb = std::function<bool(const net::PortForwarder *)>;

	class PortForwarders final : public std::list<net::PortForwarder*>
	{
	public:
		/**
		 * Allocates an empty forwarder list.
		*/
		PortForwarders();

		/**
		 * Deletes all existing forwarders in that list.
		*/
		~PortForwarders();

		/** 
		 * Deletes all port forwarders from the list having the given state.
		 * 
		 * @return the number of deleted forwarders.
		*/
		size_t delete_having_state(const state_check_cb& check_cb);

		/**
		 * Disconnects all port forwarders.
		 * @return the number of disconnected forwarders.
		*/
		size_t abort_all() const;

		/*
		 * Returns true if at least one port forwarder is trying to connect.
		*/
		bool has_connecting_forwarders() const noexcept;

		/**
		 * Returns the number of connected forwarders.
		*/
		size_t connected_count() const noexcept;

	private:
		// The class name.
		static const char* __class__;

		// a reference to the application logger
		utl::Logger* const _logger;
	};

}
