/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/

#include "net/PortForwarder.h"
#include "tools/SysUtil.h"
#include "lwip/timeouts.h"
#include "lwip/tcp.h"

namespace net {
	using namespace tools;

	PortForwarder::PortForwarder(bool tcp_nodelay, int keepalive) :
		_logger(Logger::get_logger()),
		_local_server(),
		_local_client(nullptr),
		_tcp_nodelay(tcp_nodelay),
		_keepalive(keepalive),
		_state(State::READY),
		_reply_queue(16),
		_forward_queue(16),
		_forwarded_bytes(0),
		_fflush_timeout(false),
		_rflush_timeout(false)
	{
		DEBUG_CTOR(_logger, "PortForwarder");
	}


	PortForwarder::~PortForwarder()
	{
		DEBUG_DTOR(_logger, "PortForwarder");

		// Remove all potential active timers
		sys_untimeout(timeout_cb, &_connect_timeout);
		sys_untimeout(timeout_cb, &_fflush_timeout);
		sys_untimeout(timeout_cb, &_rflush_timeout);
	}


	bool PortForwarder::connect(Listener& listener, const Endpoint& remote_endpoint)
	{
		DEBUG_ENTER(_logger, "PortForwarder", "connect");

		if (_state != State::READY) {
			_logger->error("ERROR: PortForwarder %x not in READY state", this);
			return false;
		}

		ip_addr_t addr;
		if (!remote_endpoint.is_ipaddr(addr)) {
			_logger->error("ERROR: %s is not a valid IP address, unable to forward traffic.",
				remote_endpoint.to_string().c_str());

			_state = State::FAILED;
			return false;
		}

		// Accept the connection from the local client
		mbed_err rc_accept = listener.accept(_local_server);
		if (rc_accept != 0) {
			_logger->error("ERROR: PortForwarder %x - accept error %s.",
				this,
				mbed_errmsg(rc_accept).c_str());

			_state = State::FAILED;
			return false;
		}

		// Allocate the TCP client
		_local_client = tcp_new();
		if (!_local_client) {
			_logger->error("ERROR: PortForwarder %x - memory allocation failure.", this);

			_state = State::FAILED;
			return false;
		}

		// NODELAY is not working, program hangs immediately after sending data.
		//if (_tcp_nodelay)
		//	tcp_nagle_disable(_local_client);

		if (_keepalive > 0) {
			// Turn on TCP keep alive
			ip_set_option(_local_client, SOF_KEEPALIVE);

			// Set the time between keepalive messages in milli-seconds
			_local_client->keep_idle = _keepalive;
			_local_client->keep_intvl = _keepalive;
		}

		lwip_err rc_con = tcp_connect(_local_client, &addr, remote_endpoint.port(), tcp_connected_cb);
		if (rc_con == ERR_OK) {
			_state = State::CONNECTING;

			// start a connection timer
			_connect_timeout = false;
			sys_timeout(10 * 1000, timeout_cb, &_connect_timeout);

			// configure the callbacks
			tcp_arg(_local_client, this);
			tcp_err(_local_client, tcp_err_cb);
			tcp_sent(_local_client, tcp_sent_cb);
			tcp_recv(_local_client, tcp_recv_cb);
		}
		else {
			_state = State::FAILED;

			_logger->error("ERROR: forward - %s",
				this,
				lwip_errmsg(rc_con).c_str());

			// delete the protocol control block
			tcp_close(_local_client);

			// close the local server socket
			_local_server.close();
		}

		return (rc_accept == 0) && (rc_con == 0);
	}


	void PortForwarder::disconnect()
	{
		DEBUG_ENTER(_logger, "PortForwarder", "disconnect");

		if (_state != State::CONNECTED) {
			_logger->error("ERROR: PortForwarder %x - not in CONNECTED state", this);
			return;
		}

		// Start the disconnection phase.  During this phase we will continue
		// to forward what is in the forward queue. 
		_state = State::DISCONNECTING;

		// Close our server.
		_local_server.close();

		// It is not possible to reply to the server anymore, we can clear the reply queue.
		_reply_queue.clear();

		// Start a timer.  
		sys_timeout(10 * 1000, timeout_cb, &_fflush_timeout);

		return;
	}


	void PortForwarder::abort()
	{
		DEBUG_ENTER(_logger, "PortForwarder", "abort");

		if (!(_state == State::CONNECTED || _state == State::CONNECTING)) {
			_logger->error("ERROR: PortForwarder %x - not in connected or connecting state", this);
			return;
		}
		_state = State::DISCONNECTING;

		// Abort the connection by sending a RST (reset) segment to the remote.
		// The pcb is deallocated, the function tcp_err_cb is called which
		// finally set the current state to DISCONNECTED
		tcp_abort(_local_client);
		_local_client = nullptr;

		// Clear all queues
		_forward_queue.clear();
		_reply_queue.clear();
	}



	bool PortForwarder::recv()
	{
		TRACE_ENTER(_logger, "PortForwarder", "recv");

		if (_state != State::CONNECTED) {
			_logger->error("ERROR: PortForwarder %x - not in connected state", this);
			return false;
		}

		byte buffer[2048];
		int rc = _local_server.recv(buffer, sizeof(buffer));
		if (rc <= 0) {
			return false;
		}

		pbuf* const data = pbuf_alloc(PBUF_RAW, rc, PBUF_RAM);
		if (!data) {
			_logger->error("ERROR: PortForwarder %x - memory allocation error", this);
			return false;
		}

		pbuf_take(data, buffer, rc);
		_forward_queue.append(data);
		pbuf_free(data);

		return true;
	}


	bool PortForwarder::forward()
	{
		TRACE_ENTER(_logger, "PortForwarder", "forward");

		if (_state != State::CONNECTED) {
			_logger->error("ERROR: PortForwarder %x - not in connected state", this);
			return false;
		}

		int written = 0;
		lwip_err rc = _forward_queue.write(_local_client, written);
		if (rc) {
			_logger->error("ERROR: forward - %s", lwip_errmsg(rc).c_str());
		}
		else {
			_forwarded_bytes += written;
		}


		return rc == 0;
	}


	bool PortForwarder::reply()
	{
		TRACE_ENTER(_logger, "PortForwarder", "reply");

		if (_state != State::CONNECTED) {
			_logger->error("ERROR: PortForwarder %x - not in connected state", this);
			return false;
		}

		int written;
		return _reply_queue.write(_local_server, written) == 0;
	}


	void PortForwarder::fflush()
	{
		if (_state != State::DISCONNECTING) {
			_logger->error("ERROR: PortForwarder %x - not in disconnecting state", this);
			return;
		}

		// send what we can
		int written;
		lwip_err rc = _forward_queue.write(_local_client, written);

		// Stop to forward data 
		//        if an error has occurred, 
		//     or if all data has been forwarded
		//     or if we are not able to forward in a fixed delay. 
		if (rc != ERR_OK || _fflush_timeout || _forward_queue.empty()) {
			// Useful only in case of error or timeout
			_forward_queue.clear();

			// Remove all callbacks.  We are not interested to be called on such events
			tcp_err(_local_client, nullptr);
			tcp_recv(_local_client, nullptr);

			// tcp_close never fails (see https://savannah.nongnu.org/bugs/?60757) even if
			// the documentation suggests it could.
			tcp_close(_local_client);
			_local_client = nullptr;

			// We are now disconnected.  The pcb has been deleted or will be deleted
			// later by the lwip stack.
			_state = State::DISCONNECTED;
		}

		return;
	}


	void PortForwarder::rflush()
	{
		if (_state != State::DISCONNECTING) {
			_logger->error("ERROR: PortForwarder %x - not in disconnecting state", this);
			return;
		}

		// send what we can
		int written;
		mbed_err rc = _reply_queue.write(_local_server, written);

		// Stop to reply 
		//        if an error has occurred, 
		//     or if all data has been forwarded
		//     or if we are not able to forward in a fixed delay. 
		if (rc < 0 || _rflush_timeout || _forward_queue.empty()) {
			_forward_queue.clear();
			_local_server.close();
			_state = State::DISCONNECTED;
		}
	}



	err_t tcp_connected_cb(void *arg, struct tcp_pcb *tpcb, err_t err)
	{
		PortForwarder* const pf = (PortForwarder*) arg;

		Logger* logger = pf->_logger;
		if (logger->is_debug_enabled()) {
			logger->debug(".... %x PortForwarder tcp connected err=%d", pf, err);
		}

		// We are now connected
		pf->_state = PortForwarder::State::CONNECTED;

		// Cancel the timeout. 
		sys_untimeout(timeout_cb, &pf->_connect_timeout);
		pf->_connect_timeout = false;

		return ERR_OK;
	}


	void tcp_err_cb(void* arg, err_t err)
	{
		PortForwarder* const pf = (PortForwarder*)arg;

		Logger* logger = pf->_logger;
		if (logger->is_debug_enabled()) {
			logger->debug("... %x PortForwarder tcp error err=%d", pf, err);
		}

		// An error has occurred
		pf->_state = PortForwarder::State::DISCONNECTED;

		// Close our server if not yet done
		if (pf->_local_server.connected())
			pf->_local_server.close();
	}


	err_t tcp_sent_cb(void* arg, tcp_pcb* tpcb, u16_t len)
	{
		PortForwarder* const pf = (PortForwarder*)arg;
		pf->_forwarded_bytes -= len;

		return ERR_OK;
	}


	err_t tcp_recv_cb(void* arg, tcp_pcb* tpcb, pbuf* p, err_t err)
	{
		PortForwarder* const pf = (PortForwarder*)arg;
		err_t rc = ERR_OK;
		int len = 0;

		if (p) {
			if (!pf->_local_server.connected()) {
				// The local server is disconnected. we can discard any received data
				// as it is no more possible to forward it.
				tcp_recved(tpcb, p->tot_len);
			}
			else {
				len = pf->_reply_queue.append(p);
				if (len == -1) {
					// Buffer is full
					rc = ERR_MEM;
				}
				else {
					tcp_recved(tpcb, len);
				}
			}
		}
		else if (err == ERR_OK) {
			if (pf->_state == PortForwarder::State::CONNECTED) {
				pf->_state = PortForwarder::State::DISCONNECTING;

				// Remove all callbacks.  We are not interested to be called on such events
				tcp_err(pf->_local_client, nullptr);
				tcp_recv(pf->_local_client, nullptr);

				// Close the pcb
				rc = tcp_close(tpcb);
				pf->_local_client = nullptr;

				// Nothing can be forwarded anymore.
				pf->_forward_queue.clear();

				sys_timeout(10 * 1000, timeout_cb, &pf->_rflush_timeout);
			}
		}

		Logger* const logger = pf->_logger;
		if (logger->is_trace_enabled()) {
			logger->trace(
				".... %x PortForwarder tcp rcv len=%d err=%d state=%d", 
				pf,
				len,
				err,
				pf->_state
				);
		}

		return rc;
	}

	static void timeout_cb(void* arg)
	{
		bool* timeout = (bool *)arg;

		*timeout = true;
	}
}
