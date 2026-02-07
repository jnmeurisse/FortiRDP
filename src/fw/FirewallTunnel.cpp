/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022-2025 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "FirewallTunnel.h"
#include "util/Logger.h"
#include <stdexcept>
#include <utility>


namespace fw {

	FirewallTunnel::FirewallTunnel(http::HttpsClientPtr tunnel, const net::tunneler_config& config, const http::Cookies& cookie_jar) :
		net::Tunneler(*tunnel, config),
		_logger(utl::Logger::get_logger()),
		_tunnel_socket{ std::move(tunnel) },
		_cookie_jar{ cookie_jar }
	{
		DEBUG_CTOR(_logger);
	}


	FirewallTunnel::~FirewallTunnel()
	{
		DEBUG_DTOR(_logger);
	}


	bool FirewallTunnel::start()
	{
		DEBUG_ENTER(_logger);

		try {
			_tunnel_socket->connect();
			start_tunnel_mode();
		}
		catch (const std::runtime_error& e) {
			_logger->error("ERROR: failed to open the tunnel");
			_logger->error("ERROR: %s", e.what());

			return false;
		}

		// start the thread
		return net::Tunneler::start();
	}


	void FirewallTunnel::start_tunnel_mode()
	{
		DEBUG_ENTER(_logger);

		const http::Url tunnel_url{ _tunnel_socket->make_url("/remote/sslvpn-tunnel") };
		http::Request request{ http::Request::GET_VERB, tunnel_url, _cookie_jar };
		request.headers().set("Host", "sslvpn");

		_tunnel_socket->send_request(request);
	}


	const char* FirewallTunnel::__class__ = "FirewallTunnel";
}
