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
#include "tools/Counters.h"
#include "tools/Thread.h"
#include "tools/Logger.h"
#include "tools/Event.h"


namespace net {

	using namespace tools;

	struct tunneler_config {
		bool tcp_nodelay;
		int  max_clients;
		int  connect_timeout;
	};

	class Tunneler : public tools::Thread
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
		explicit Tunneler(TlsSocket& tunnel, const Endpoint& local, const Endpoint& remote,
			const tunneler_config& config);
		
		/**
		* Tunneler destructor
		*/
		virtual ~Tunneler() override;

		/**
		* Deleted copy constructor and copy assignment operator
		* to prevent copying of this object.
		*/
		Tunneler(const Tunneler&) = delete;
		Tunneler& operator=(const Tunneler&) = delete;

		// Valid states of the tunneler
		enum State { 
			READY,				// initial state
			CONNECTING,			// Establishing the PPP connection 
			RUNNING,			// Forwarding traffic and accepting new connection
			CLOSING,			// Closing all local connections 
			DISCONNECTING,		// Disconnecting the PPP interface
			STOPPED };

		/**
		 * Starts the tunnel listener
		*/
		virtual bool start() override;

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
		inline State get_state() const { return _state; }

		/**
		 * Returns the transmitted/received bytes counters.
		*/
		inline const Counters& counters() const { return _counters; }

		/**
		 * Returns the local endpoint address and port.
		*/
		inline const Endpoint& local_endpoint() const { return _listener.endpoint(); }

	protected:
		virtual unsigned int run() override;

	private:
		void compute_sleep_time(timeval &timeout) const;

		// The class name
		static const char* __class__;

		// a reference to the application logger.
		tools::Logger* const _logger;

		// Tunneler configuration.
		const tunneler_config _config;

		// The current state of this tunnel listener.
		volatile State _state;

		// The tunneler must stop when this flag is set.
		volatile bool _terminate;

		// Tunnel socket.
		TlsSocket&  _tunnel;

		// Counters of bytes sent to / received from the tunnel.
		Counters _counters;

		// PP interface.
		PPInterface _pp_interface;
		
		// This event is set when the tunneler is listening.
		Event _listening_status;

		// The local end point and an associated listener.
		const Endpoint _local_endpoint;
		Listener _listener;

		// The remote end point (protected by the firewall).
		Endpoint _remote_endpoint;
	};

}
