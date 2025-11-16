/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Tunneler.h"

#include <windows.h>
#include <list>
#include "net/DnsClient.h"
#include "net/PortForwarders.h"
#include "tools/ErrUtil.h"


static void timeout_cb(void* arg)
{
	bool* timeout = reinterpret_cast<bool*>(arg);
	*timeout = true;
}


namespace net {

	Tunneler::Tunneler(TlsSocket& tunnel, const Endpoint& local_ep, const Endpoint& remote_ep,
		const tunneler_config& config) :
		tools::Thread(),
		_logger(Logger::get_logger()),
		_config(config),
		_state(State::READY),
		_terminate(false),
		_tunnel(tunnel),
		_counters(),
		_pp_interface(tunnel, _counters),
		_listener(),
		_listening_status(),
		_local_endpoint(local_ep),
		_remote_endpoint(remote_ep)
	{
		DEBUG_CTOR(_logger);
	}


	Tunneler::~Tunneler()
	{
		DEBUG_DTOR(_logger);
	}


	bool Tunneler::start()
	{
		DEBUG_ENTER(_logger);
		bool started = true;

		mbed_err rc = _listener.bind(_local_endpoint);

		if (rc < 0) {
			_logger->error("ERROR: listener error on %s", _local_endpoint.to_string().c_str());
			_logger->error("%s", mbed_errmsg(rc).c_str());
		
			started = false;
		}
		else {
			started = Thread::start();
		}

		if (!started) {
			_state = State::STOPPED;
		}

		return started;
	}


	void Tunneler::terminate()
	{
		DEBUG_ENTER(_logger);
		_terminate = true;
	}


	bool Tunneler::wait_listening(DWORD timeout)
	{
		return _listening_status.wait(timeout) && _listener.is_ready();
	}


	unsigned int Tunneler::run()
	{
		DEBUG_ENTER(_logger);

		int rc = 0;
		bool stop = false;
		FD_SET read_set;
		FD_SET write_set;
		timeval timeout;
		PortForwarders active_port_forwarders;
		bool connecting = false;
		bool abort_timeout = false;
		bool disconnect_timeout = false;

		_logger->info(">> starting tunnel");
		_state = State::CONNECTING;


		// Disable Nagle algorithm if required
		_tunnel.set_nodelay(_config.tcp_nodelay);

		if (!_pp_interface.open()) {
			_state = State::STOPPED;
			return 0;
		}

		while (!stop) {
			FD_ZERO(&read_set);
			FD_ZERO(&write_set);

			// Define select conditions only if the tunnel is still connected.
			if (_tunnel.is_connected()) {
				if (_pp_interface.must_transmit()) {
					// data is available in the output queue, check if we can write.
					FD_SET(_tunnel.get_fd(), &write_set);
				}

				// always check if data is available from the tunnel.
				FD_SET(_tunnel.get_fd(), &read_set);

				if (_pp_interface.if4_up() && !connecting && 
					active_port_forwarders.connected_count() < _config.max_clients) {
					// We are ready to accept a new connection only if the PPP interface
					// is up, if we are not currently accepting a connection and the 
					// max number of connected forwarders is not reached.
					FD_SET(_listener.get_fd(), &read_set);
				}

				for (auto pf : active_port_forwarders) {
					const int fd = pf->get_fd();

					if (pf->connected()) {
						// Do we have data to send or to reply ?
						if (pf->can_receive())
							FD_SET(fd, &read_set);

						if (pf->must_reply())
							FD_SET(fd, &write_set);
					}
					else if (pf->disconnecting()) {
						// Can we send data still in the output queue ?
						if (pf->can_rflush())
							FD_SET(fd, &write_set);
					}
				}

				// Determine how long we sleep in the select.
				compute_sleep_time(timeout);

				// Wait for a network event or timeout.
				rc = select(0, &read_set, &write_set, NULL, &timeout);
				if (rc > 0) {
					if (FD_ISSET(_tunnel.get_fd(), &write_set)) {
						// Send PPP through the tunnel 
						if (!_pp_interface.send()) {
							_tunnel.shutdown();
							terminate();
						}
					}

					if (FD_ISSET(_tunnel.get_fd(), &read_set)) {
						// Receive PPP data from the tunnel.
						if (!_pp_interface.recv()) {
							_logger->info(">> tunnel closed by peer");

							_tunnel.shutdown();
							terminate();
						}
					}

					if (FD_ISSET(_listener.get_fd(), &read_set)) {
						// Accept a new connection.
						PortForwarder* pf = new PortForwarder(_remote_endpoint, _config.tcp_nodelay, 30 * 1000);

						if (pf->connect(_listener)) {
							// A new port forwarder is active.
							connecting = true;
							active_port_forwarders.push_back(pf);
						}
						else {
							delete pf;
						}
					}

					// Transmit data to and from each port forwarder to the local socket.
					for (auto pf : active_port_forwarders) {
						const int fd = pf->get_fd();

						if (pf->connected()) {
							if (FD_ISSET(fd, &read_set)) {
								if (!pf->recv())
									pf->disconnect();
							}

							if (FD_ISSET(fd, &write_set)) {
								if (!pf->reply())
									pf->disconnect();
							}
						}
						else if (pf->disconnecting()) {
							if (FD_ISSET(fd, &write_set)) {
								if (pf->can_rflush())
									pf->rflush();
							}
						}
					}

				}
				else if (rc == 0) {
					// timeout, noop

				}
				else if (rc == SOCKET_ERROR) {
					// an error in the select has been detected, it is a fatal error
					_logger->error("ERROR: socket select error=%d", WSAGetLastError());
					terminate();
				}
			}

			// Forward data through the local IP stack. LwIP generates IP frames
			// that are appended to the PPP interface's output queue.
			// The pppossl_netif_output function is called, which in turn calls
			// the ppp_output_cb callback registered when the PPP interface was created.
			for (auto pf : active_port_forwarders) {
				if (pf->ctimeout()) {
					// Abort all forwarders in connection time out
					pf->abort();
				}

				if (pf->connected()) {
					if (pf->must_forward()) {
						if (!pf->forward())
							pf->disconnect();
					}
				}
				else if (pf->disconnecting()) {
					if (pf->can_fflush())
						pf->fflush();
				}
			}

			sys_check_timeouts();

			// Delete all failed port forwarders
			active_port_forwarders.delete_having_state([](const PortForwarder* pf) {return pf->failed(); });

			// Delete all closed port forwarders
			active_port_forwarders.delete_having_state([](const PortForwarder* pf) {return pf->disconnected(); });

			switch (_state) {
			case State::CONNECTING:
				if (_terminate) {
					_state = State::CLOSING;
				}
				else if (_pp_interface.if4_up()) {
					// The listener is now accepting inbound connection.
					_listening_status.set();

					_state = State::RUNNING;
					_logger->info(">> tunnel is up, listening on %s",
						_listener.endpoint().to_string().c_str());
					_logger->info("     IP=%s/%d GW=%s MTU=%d",
						_pp_interface.addr().c_str(),
						_pp_interface.netmask(),
						_pp_interface.gateway().c_str(),
						_pp_interface.mtu());

					if (DnsClient::configured()) {
						_logger->info("     DNS=%s", DnsClient::dns().c_str());
					}
				}
				break;

			case State::RUNNING:
				if (_terminate) {
					_state = State::CLOSING;
					
					// Abort all port forwarders (send a RST packet)
					abort_timeout = false;
					if (active_port_forwarders.abort_all() > 0) {
						// Delay shutting down the PPP interface for 1 second to allow
						// the RST packet to be received by the peer.
						sys_timeout(1000, timeout_cb, &abort_timeout);
					}
				}
				else {
					_pp_interface.send_keep_alive();

					// A port forwarder connection was started.
					if (connecting) {
						// Is the connection still pending ?
						connecting = active_port_forwarders.has_connecting_forwarders();
					}
				}

				break;

			case State::CLOSING:
				if (active_port_forwarders.size() == 0 || abort_timeout) {
					// All connections are closed, shutdown the ppp interface
					_state = State::DISCONNECTING;
					_pp_interface.close(!_tunnel.is_connected());

					// Set a timer to ensure the thread exits. The timeout is deliberately
					// longer than SyncDisconnect's timeout. If the interface remains active,
					// a PPP memory descriptor could be leaked, preventing the PPP interface
					// from being restarted.
					disconnect_timeout = false;
					sys_timeout(50 * 1000, timeout_cb, &disconnect_timeout);
				}
				break;

			case State::DISCONNECTING:
				// Wait until PPP interface is in dead state.
				if (_pp_interface.dead() || disconnect_timeout) {
					_logger->info(">> tunnel is down");
					stop = true;
				}

				break;

			default:
				break;
			}
		}

		// Free all resources used by the PPP interface.
		_pp_interface.release();
		sys_untimeout(timeout_cb, &abort_timeout);
		sys_untimeout(timeout_cb, &disconnect_timeout);

		// Close the listening socket.
		_listener.close();

		// Shutdown the tunnel socket.
		_tunnel.shutdown();

		_logger->debug("... closing tunneler stop=%d terminate=%d", 
				stop,
				_terminate);

		_state = State::STOPPED;

		return 0;
	}


	void Tunneler::compute_sleep_time(timeval &timeout) const
	{
		u32_t sleep_time = sys_timeouts_sleeptime();

		timeout.tv_sec = 0;
		if (sleep_time == SYS_TIMEOUTS_SLEEPTIME_INFINITE || sleep_time > 500) {
			timeout.tv_usec = 500  * 1000;
		} 
		else {
			timeout.tv_usec = sleep_time * 1000;
		}

		return;
	}

	const char* Tunneler::__class__ = "Tunneler";

}


