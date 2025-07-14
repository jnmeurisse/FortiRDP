/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022-2025 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "FirewallTunnel.h"
#include "tools/Logger.h"


namespace fw {
	using namespace tools;

	FirewallTunnel::FirewallTunnel(http::HttpsClientPtr tunnel_socket,
		const net::Endpoint& local_ep, const net::Endpoint& remote_ep,
		const net::tunneler_config& config, const http::Cookies& cookie_jar
	) :
		net::Tunneler(*tunnel_socket, local_ep, remote_ep, config),
		_logger(Logger::get_logger()),
		_tunnel_socket{ std::move(tunnel_socket) },
		_cookie_jar{ cookie_jar }
	{
	}


	bool FirewallTunnel::start()
	{
		try {
			// Open the encrypted tunnel
			_tunnel_socket->connect();

			// start the tunnel mode
			start_tunnel_mode();
		}
		catch (const mbed_error& e) {
			_logger->error("ERROR: failed to open the tunnel");
			_logger->error("ERROR: %s", e.message().c_str());

			return false;
		}

		// start the thread
		return net::Tunneler::start();
	}


	void FirewallTunnel::start_tunnel_mode()
	{
		const http::Url tunnel_url{ _tunnel_socket->make_url("/remote/sslvpn-tunnel") };
		http::Request request{ http::Request::GET_VERB, tunnel_url, _cookie_jar };
		request.headers().set("Host", "sslvpn");

		_tunnel_socket->send_request(request);
	}

}
