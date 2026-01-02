/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022-2025 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "http/HttpsClient.h"
#include "http/Cookies.h"
#include "net/Endpoint.h"
#include "net/Tunneler.h"


namespace fw {

	class FirewallTunnel final : public net::Tunneler
	{
	public:
		/**
		* Creates a Firewall tunnel instance.
		*
		* The Tunnel forwards traffic received on the specified local endpoint
		* to the remote endpoint through a secure, encrypted tunnel.
		*
		* @param tunnel_socket  The TLS socket used for secure communication.
		* @param local  The local network endpoint to listen for incoming traffic.
		* @param remote  The remote network endpoint to forward traffic to.
		* @param config  Configuration settings for the tunneler.
		* @param cookie_jar Session cookies
		*/
		FirewallTunnel(http::HttpsClientPtr tunnel_socket, const net::Endpoint& local_ep,
			const net::Endpoint& remote_ep, const net::tunneler_config& config, const http::Cookies& cookie_jar);
		~FirewallTunnel() override;


		/**
		 * Starts the tunneler.
		 * 
		 * The function opens an encrypted TLS socket and starts the tunnel.
		 */
		virtual bool start() override;

	private:
		// The class name
		static const char* __class__;

		// A reference to the application logger.
		tools::Logger* const _logger;

		// The encrypted socket.
		const http::HttpsClientPtr _tunnel_socket;

		// The application cookie jar
		const http::Cookies& _cookie_jar;

		/* Starts the tunnel mode.
		 *
		 * The function initiates the tunnel mode by sending the appropriate URL
		 * and session cookie.
		 *
		*/
		void start_tunnel_mode();
	};

}