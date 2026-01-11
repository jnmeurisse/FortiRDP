/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/

#include "PortForwarder.h"

#include <algorithm>
#include <array>
#include <lwip/err.h>
#include <lwip/timeouts.h>
#include <lwip/tcp.h>
#include "net/DnsClient.h"


namespace net {
	using namespace utl;


	// lwip callbacks
	void dns_found_cb(const char* name, const ip_addr_t* ipaddr, void* callback_arg);
	err_t tcp_connected_cb(void* arg, struct tcp_pcb* tpcb, err_t err);
	void tcp_err_cb(void* arg, err_t err);
	err_t tcp_sent_cb(void* arg, tcp_pcb* tpcb, u16_t len);
	err_t tcp_recv_cb(void* arg, tcp_pcb* tpcb, pbuf* p, err_t err);
	void timeout_cb(void* arg);


	PortForwarder::PortForwarder(const net::Endpoint& endpoint, bool tcp_nodelay, int keepalive) :
		_logger(Logger::get_logger()),
		_state(State::READY),
		_endpoint(endpoint),
		_tcp_nodelay(tcp_nodelay),
		_keepalive(keepalive),
		_local_server(),
		_local_client(nullptr),
		_connect_timeout(false),
		_fflush_timeout(false),
		_rflush_timeout(false),
		_reply_queue(8 * 1024),
		_forward_queue(8 * 1024),
		_forwarded_bytes(0)
	{
		DEBUG_CTOR(_logger);
	}


	PortForwarder::~PortForwarder()
	{
		DEBUG_DTOR(_logger);

		// Remove all potential active timers
		sys_untimeout(timeout_cb, &_connect_timeout);
		sys_untimeout(timeout_cb, &_fflush_timeout);
		sys_untimeout(timeout_cb, &_rflush_timeout);
	}


	bool PortForwarder::connect(net::Listener& listener)
	{
		DEBUG_ENTER(_logger);

		if (_state != State::READY) {
			_logger->error("ERROR: forwarder %d not in READY state", get_fd());
			return false;
		}

		// Accept the connection from a local client.
		const mbed_err rc_accept = listener.accept(_local_server);
		if (rc_accept != 0) {
			_logger->error("ERROR: %s 0x%012Ix - accept error (%s)",
				__class__,
				PTR_VAL(this),
				mbed_errmsg(rc_accept).c_str()
			);

			_state = State::FAILED;
			return false;
		}

		// Disable Nagle algorithm on the local server.
		_local_server.set_nodelay(_tcp_nodelay);

		// Resolve the end point host name to an IP address.  The DNS request
		// is sent to the FortiGate firewall and is asynchronous.  dns_found_cb is 
		// called when the host name is resolved or if the resolution fails.
		ip_addr_t addr;
		const lwip_err rc_query = DnsClient::query(_endpoint.hostname(), addr, dns_found_cb, this);
		if (rc_query == ERR_OK || rc_query == ERR_INPROGRESS) {
			// host name is already resolved or not yet resolved.
			_state = State::CONNECTING;
		}
		else if (rc_query == ERR_VAL) {
			// DNS server is not configured, abort the connection.
			_state = State::FAILED;
			_logger->error("ERROR: %s 0x%012Ix - can not resolve %s",
				__class__,
				PTR_VAL(this),
				_endpoint.hostname().c_str()
			);
		}
		else {
			// There was an error during name resolution, abort the connection.
			_state = State::FAILED;
			_logger->error("ERROR: %s 0x%012Ix - DNS error (%s)",
				__class__,
				PTR_VAL(this),
				lwip_errmsg(rc_query).c_str()
			);
		}

		if (_state == State::FAILED)
			return false;

		// Allocate the TCP client.
		_local_client = tcp_new();
		if (!_local_client) {
			_logger->error("ERROR: %s 0x%012Ix - tcp_new memory allocation failure",
				__class__,
				PTR_VAL(this)
			);

			_state = State::FAILED;
			return false;
		}

		// Set TCP_NODELAY inside the tunnel
		if (_tcp_nodelay)
			tcp_nagle_disable(_local_client);

		if (_keepalive > 0) {
			// Turn on TCP keep alive
			ip_set_option(_local_client, SOF_KEEPALIVE);

			// Set the time between keep alive messages in milli-seconds
			_local_client->keep_idle = _keepalive;
			_local_client->keep_intvl = _keepalive;
		}

		if (rc_query == ERR_OK) {
			// Host name is resolved.
			dns_found_cb(_endpoint.hostname().c_str(), &addr, this);
		}

		return true;
	}


	void PortForwarder::disconnect()
	{
		DEBUG_ENTER(_logger);

		if (_state != State::CONNECTED)
			return;

		// Start the disconnection phase.  During this phase we will continue
		// to forward bytes available is in the forward queue. 
		_state = State::DISCONNECTING;

		// Close our TCP server.
		_local_server.close();

		// As it is not possible to reply to the remote server anymore,
		// we clear the reply queue.
		_reply_queue.clear();

		// Start a timer
		sys_timeout(10 * 1000, timeout_cb, &_fflush_timeout);

		return;
	}


	void PortForwarder::abort()
	{
		DEBUG_ENTER(_logger);

		if (!(_state == State::CONNECTED || _state == State::CONNECTING)) {
			_logger->error("ERROR: %s 0x%012Ix - not in connected or connecting state",
				__class__,
				PTR_VAL(this)
			);

			return;
		}
		_state = State::DISCONNECTING;

		// Abort the connection by sending a RST (reset) segment to the remote host.
		// The TCP PCB is de-allocated, the function tcp_err_cb is called which
		// finally set the current state to DISCONNECTED.
		::tcp_abort(_local_client);
		_local_client = nullptr;

		// Clear all queues
		_forward_queue.clear();
		_reply_queue.clear();
	}


	bool PortForwarder::recv()
	{
		TRACE_ENTER(_logger);

		if (_state != State::CONNECTED)
			return false;

		std::array<unsigned char, 2048> incoming_data = {};

		const size_t available_space = std::min(incoming_data.size(), _forward_queue.remaining_space());
		if (available_space == 0) {
			// There is no space in the queue to store data that could be
			// available in the socket.
			return true;
		}

		const rcv_status status{ _local_server.recv_data(incoming_data.data(), available_space)};
		if (status.code != rcv_status_code::NETCTX_RCV_OK) {
			return false;
		}

		// The number of bytes received is less than 2048, so the cast is safe.
		const u16_t length = static_cast<u16_t>(status.rbytes);
		pbuf* const buffer = ::pbuf_alloc(PBUF_RAW, length, PBUF_RAM);
		if (!buffer) {
			_logger->error("ERROR: %s 0x%012Ix - pbuf memory allocation error",
				__class__,
				PTR_VAL(this)
			);
			return false;
		}

		// Copy received data into the buffer.
		::pbuf_take(buffer, incoming_data.data(), length);

		// If the local sender provides fewer bytes than the buffer capacity,
		// it is assumed no more data will arrive immediately, so the data
		// must be forwarded with the TCP Push (PSH) flag.
		if (length != available_space)
			buffer->flags = PBUF_FLAG_PUSH;

		// Append the buffer to the queue.
		if (!_forward_queue.push(buffer)) {
			_logger->error("INTERNAL ERROR: %s 0x%012Ix - forward queue data full",
				__class__,
				PTR_VAL(this)
			);

			return false;
		}

		// Decrement the reference counter and free the buffer if it drops to 0.
		::pbuf_free(buffer);

		return true;
	}


	bool PortForwarder::forward()
	{
		TRACE_ENTER(_logger);

		if (_state != State::CONNECTED) 
			return false;

		size_t written = 0;
		const lwip_err rc = _forward_queue.write(_local_client, written);
		if (rc) {
			_logger->error("ERROR: %s 0x%012Ix - %s",
				__class__,
				PTR_VAL(this),
				lwip_errmsg(rc).c_str()
			);
		}
		else {
			_forwarded_bytes += written;
		}

		return rc == 0;
	}


	bool PortForwarder::reply()
	{
		TRACE_ENTER(_logger);

		if (_state != State::CONNECTED) 
			return false;

		size_t written = 0;
		const mbed_err rc = _reply_queue.write(_local_server, written);
		if (rc) {
			_logger->error("ERROR: %s 0x%012Ix - %s",
				__class__,
				PTR_VAL(this),
				mbed_errmsg(rc).c_str()
			);
		}

		return rc == 0;
	}


	void PortForwarder::flush_forward_queue()
	{
		if (_state != State::DISCONNECTING) {
			_logger->error("ERROR: %s 0x%012Ix - not in disconnecting state",
				__class__,
				PTR_VAL(this)
			);

			return;
		}

		// send what we can
		size_t written;
		lwip_err rc = _forward_queue.write(_local_client, written);

		// Stop to forward data 
		//        if an error has occurred, 
		//     or if all data has been forwarded
		//     or if we are not able to forward in a fixed delay. 
		if (rc != ERR_OK || _fflush_timeout || _forward_queue.is_empty()) {
			// Useful only in case of error or timeout
			_forward_queue.clear();

			// Remove all callbacks.  We are not interested to be called on such events.
			::tcp_err(_local_client, nullptr);
			::tcp_recv(_local_client, nullptr);

			// tcp_close never fails (see https://savannah.nongnu.org/bugs/?60757) even if
			// the documentation suggests it could.
			::tcp_close(_local_client);
			_local_client = nullptr;

			// We are now disconnected.  The TCP PCB has been deleted or will be deleted
			// later by the lwIP stack.
			_state = State::DISCONNECTED;
		}

		return;
	}


	void PortForwarder::flush_reply_queue()
	{
		if (_state != State::DISCONNECTING) {
			_logger->error("ERROR: %s 0x%012Ix - not in disconnecting state",
				__class__,
				PTR_VAL(this)
			);

			return;
		}

		// Attempt to send all available data; the actual amount sent depends on
		// the underlying network capacity.
		size_t written = 0;
		const mbed_err rc = _reply_queue.write(_local_server, written);

		// Disconnects this forwarder
		//        if an error has occurred,
		//     or if all data has been forwarded
		//     or if we are not able to forward in a fixed delay 
		if (rc != 0 || _rflush_timeout || _forward_queue.is_empty()) {
			_forward_queue.clear();
			_local_server.close();
			_state = State::DISCONNECTED;
		}
	}


	void dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
	{
		auto pf = static_cast<PortForwarder*>(callback_arg);

		if (pf->_endpoint.hostname().compare(name) != 0) {
			pf->_state = PortForwarder::State::FAILED;
			pf->_logger->error(
				"ERROR: DNS response for wrong host name %s",
				name);

			return;
		}

		if (ipaddr == nullptr) {
			pf->_state = PortForwarder::State::FAILED;
			pf->_logger->error(
				"ERROR: can not resolve host %s, DNS query failed",
				name);

			return;
		}

		lwip_err rc_con = ::tcp_connect(pf->_local_client, ipaddr, pf->_endpoint.port(), tcp_connected_cb);
		if (rc_con == ERR_OK) {
			// Start a connection timer
			::sys_timeout(10 * 1000, timeout_cb, &pf->_connect_timeout);

			// Configure the callbacks
			::tcp_arg(pf->_local_client, pf);
			::tcp_err(pf->_local_client, tcp_err_cb);
			::tcp_sent(pf->_local_client, tcp_sent_cb);
			::tcp_recv(pf->_local_client, tcp_recv_cb);
		}
		else {
			pf->_state = PortForwarder::State::FAILED;

			pf->_logger->error("ERROR: forward - %s",
				pf,
				lwip_errmsg(rc_con).c_str());

			// Delete the TCP PCB.
			::tcp_close(pf->_local_client);

			// Close the local server socket.
			pf->_local_server.close();
		}
	}


	err_t tcp_connected_cb(void *arg, struct ::tcp_pcb *tpcb, err_t err)
	{
		LWIP_UNUSED_ARG(tpcb);
		auto pf = static_cast<PortForwarder*>(arg);

		Logger* logger = pf->_logger;
		if (logger->is_debug_enabled())
			logger->debug("PortForwarder 0x%012Ix TCP connected err=%d", PTR_VAL(pf), err);

		// We are now connected.
		pf->_state = PortForwarder::State::CONNECTED;

		// Cancel the timeout.
		sys_untimeout(timeout_cb, &pf->_connect_timeout);
		pf->_connect_timeout = false;

		return ERR_OK;
	}


	void tcp_err_cb(void* arg, err_t err)
	{
		auto pf = static_cast<PortForwarder*>(arg);

		Logger* logger = pf->_logger;
		logger->debug("... 0x%012Ix PortForwarder TCP error err=%d", PTR_VAL(pf), err);

		if (err != ERR_OK) {
			if (pf->_state == PortForwarder::State::DISCONNECTING && pf->_connect_timeout) {
				logger->error("ERROR: timeout, can't connect to %s", 
					pf->_endpoint.to_string().c_str());
			}
			else if (pf->_state != PortForwarder::State::DISCONNECTING) {
				logger->error("ERROR: %s", lwip_errmsg(err).c_str());
			}
		}

		// An error has occurred.
		pf->_state = PortForwarder::State::DISCONNECTED;

		// Close our server if not yet done.
		if (pf->_local_server.is_connected())
			pf->_local_server.close();
	}


	err_t tcp_sent_cb(void* arg, tcp_pcb* tpcb, u16_t len)
	{
		LWIP_UNUSED_ARG(tpcb);
		auto pf = static_cast<PortForwarder*>(arg);
		pf->_forwarded_bytes -= len;

		return ERR_OK;
	}


	err_t tcp_recv_cb(void* arg, tcp_pcb* tpcb, pbuf* p, err_t err)
	{
		auto pf = static_cast<PortForwarder*>(arg);
		err_t rc = ERR_OK;
		uint16_t len = 0;
		Logger* const logger = pf->_logger;

		if (logger->is_trace_enabled()) {
			logger->trace(
				"PortForwarder 0x%012Ix tcp_rcv_cb", PTR_VAL(pf), pf->_state);
		}

		if (p) {
			len = p->tot_len;

			if (!pf->_local_server.is_connected()) {
				// The local server is disconnected, we can discard any received data.
				// It is no longer possible to forward it.
				::tcp_recved(tpcb, len);
				::pbuf_free(p);
			}
			else {
				if (!pf->_reply_queue.push(p)) {
					// Buffer is full
					rc = ERR_MEM;
				}
				else {
					// len bytes have been received
					::tcp_recved(tpcb, len);

					// the buffer is now in the queue, we can free it.
					::pbuf_free(p);
				}
			}
		}
		else if (err == ERR_OK) {
			if (pf->_state == PortForwarder::State::CONNECTED) {
				// pbuf is NULL which indicate that the remote host has closed the connection.
				pf->_state = PortForwarder::State::DISCONNECTING;

				// Remove all callbacks.  We are not interested to be called on such events
				::tcp_err(pf->_local_client, nullptr);
				::tcp_recv(pf->_local_client, nullptr);

				// Close the TCP PCB.
				rc = ::tcp_close(tpcb);
				pf->_local_client = nullptr;

				// Nothing can be forwarded anymore.
				pf->_forward_queue.clear();

				::sys_timeout(10 * 1000, timeout_cb, &pf->_rflush_timeout);
			}
		}

		if (logger->is_trace_enabled()) {
			logger->trace(
				"PortForwarder 0x%012Ix tcp_rcv_cb len=%d err=%d state=%d", 
				PTR_VAL(pf),
				len,
				err,
				pf->_state
			);
		}

		return rc;
	}


	void timeout_cb(void* arg)
	{
		auto timeout = static_cast<bool*>(arg);
		*timeout = true;
	}


	const char* PortForwarder::__class__ = "PortForwarder";
}
