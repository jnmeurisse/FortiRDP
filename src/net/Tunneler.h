/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "net/Endpoint.h"
#include "net/TlsSocket.h"
#include "net/Listener.h"
#include "net/PPInterface.h"
#include "util/Counters.h"
#include "util/Thread.h"
#include "util/Logger.h"
#include "util/Event.h"


namespace net {

	struct tunneler_config {
		bool tcp_nodelay;
		int  max_clients;
		int  connect_timeout;
	};

	class Tunneler : public utl::Thread
	{
	public:
		/**
		* Creates a Tunneler instance.
		*
		* The Tunneler forwards traffic received on the specified local endpoint
		* to the remote endpoint through a secure, encrypted tunnel.
		*
		* @param tunnel  The TLS socket used for secure communication.
		* @param local   The local network endpoint to listen for incoming traffic.
		* @param remote  The remote network endpoint to forward traffic to.
		* @param config  Configuration settings for the tunneler.
		*/
		explicit Tunneler(net::TlsSocket& tunnel, const net::Endpoint& local, const net::Endpoint& remote,
			const tunneler_config& config);
		
		/**
		* Tunneler destructor
		*/
		~Tunneler() override;

		/**
		* Deleted copy constructor and copy assignment operator
		* to prevent copying of this object.
		*/
		Tunneler(const Tunneler&) = delete;
		Tunneler& operator=(const Tunneler&) = delete;

		// Valid states of the tunneler
		enum class State { 
			READY,				// initial state
			CONNECTING,			// Establishing the PPP connection 
			RUNNING,			// Forwarding traffic and accepting new connection
			CLOSING,			// Closing all local connections 
			DISCONNECTING,		// Disconnecting the PPP interface
			STOPPED };

		/**
		 * Starts the tunnel listener
		*/
		bool start() override;

		/**
		 * Terminates the tunnel listener.
		*/
		void terminate();

		/**
		 * Waits until the tunneler is in listening mode.
		*/
		bool wait_listening(DWORD timeout) const;

		/**
		 * Returns the state of the listener.
		*/
		inline State get_state() const noexcept { return _state; }

		/**
		 * Returns the transmitted/received bytes counters.
		*/
		inline const utl::Counters& counters() const noexcept { return _counters; }

		/**
		* Returns the number of active clients
		*/
		inline size_t clients_count() const noexcept { return _clients_count; }

		/**
		 * Returns the local endpoint address and port.
		*/
		inline const net::Endpoint& local_endpoint() const { return _listener.endpoint(); }

	protected:
		unsigned int run() override;

	private:
		// The class name
		static const char* __class__;

		// a reference to the application logger.
		utl::Logger* const _logger;

		// Tunneler configuration.
		const tunneler_config _config;

		// The current state of this tunnel listener.
		volatile State _state;

		// The tunneler must stop when this flag is set.
		volatile bool _terminate;

		// Tunnel socket.
		net::TlsSocket&  _tunnel;

		// Counters of bytes sent to / received from the tunnel.
		utl::Counters _counters;

		// Counters of connected clients
		size_t _clients_count;

		// PP interface.
		net::PPInterface _pp_interface;
		
		// This event is set when the tunneler is listening.
		utl::Event _listening_status;

		// The local end point and an associated listener.
		const net::Endpoint _local_endpoint;
		net::Listener _listener;

		// The remote end point (protected by the firewall).
		net::Endpoint _remote_endpoint;

		void compute_sleep_time(timeval& timeout) const;
		void shutdown_tunnel();
	};

}
