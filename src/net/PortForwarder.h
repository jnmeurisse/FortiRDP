/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <lwip/tcp.h>
#include "net/Socket.h"
#include "net/Listener.h"
#include "net/Endpoint.h"
#include "net/WinsOutputQueue.h"
#include "net/LwipOutputQueue.h"
#include "tools/Logger.h"


namespace net {

	class PortForwarder final {
	public:
		explicit PortForwarder(const Endpoint& endpoint, bool tcp_nodelay, int keepalive);
		~PortForwarder();

		/*
		*/
		bool connect(Listener& listener);
		
		/*
		*/
		void disconnect();

		/*
		*/
		void abort();

		/* Returns true if this forwarder is connected
		*/
		inline bool connected() const noexcept { return _state == State::CONNECTED; }

		/* Returns true if this forwarder is in the connecting phase
		*/
		inline bool connecting() const noexcept { return _state == State::CONNECTING; }

		/* Returns true if the connection has timed out
		*/
		inline bool ctimeout() const noexcept { return _state == State::CONNECTING && _connect_timeout; }

		/* Returns true if this forwarder is in the disconnecting phase
		*/
		inline bool disconnecting() const noexcept { return _state == State::DISCONNECTING; }

		/* Returns true if this forwarder was not able to connect.
		*/
		inline bool failed() const noexcept { return _state == State::FAILED; }

		/* Returns true if this forwarder is disconnected
		*/
		inline bool disconnected() const noexcept { return _state == State::DISCONNECTED; }

		/* Returns true if this forwarder has space in the forward queue.
		*/
		inline bool can_receive() const noexcept { return !_forward_queue.full(); }

		/* Returns true if this forwarder has data in the forward queue and
		*  the tcp send buffer is not full. 
		*/
		inline bool must_forward() const { return !_forward_queue.empty() && tcp_sndbuf(_local_client) > 0; }

		/* Returns true if this forwarder has data in the reply queue
		*/
		inline bool must_reply() const noexcept { return _reply_queue.len() > 0; }

		/* Returns true if this forwarder can still flush the reply queue
		*/
		inline bool can_rflush() const noexcept { return  _local_server.connected(); }

		/* Returns true if this forwarder can still flush the forward queue
		*/
		inline bool can_fflush() const noexcept { return _local_client != nullptr; }

		/* Returns the underlying socket file descriptor
		*/
		inline int get_fd() const noexcept { return _local_server.get_fd(); }
		
		/* try to receive data from the local client and store it in the 
		*  forward queue. 
		*/
		bool recv();

		/* try to forward data to the remote server.
		*/
		bool forward();
		
		/* try to reply data to the local client
		*/
		bool reply();

		/* try to flush the forward queue
		*/
		void fflush();

		/* try to flush the reply queue
		*/
		void rflush();

	private:
		// forwarder states
		enum State {
			READY,					// The forwarder is allocated and ready 
			FAILED,					// The forwarder failed to connect
			CONNECTING,				// The forwarder is connecting
			CONNECTED,				// The forwarder is connected
			DISCONNECTING,			// The forwarder is disconnecting
			DISCONNECTED			// The forwarder is disconnected
		};

		friend void  dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg);
		friend err_t tcp_connected_cb(void *arg, struct tcp_pcb *tpcb, err_t err);
		friend void  tcp_err_cb(void *arg, err_t err);
		friend err_t tcp_sent_cb(void *arg, struct tcp_pcb *tpcb, u16_t len);
		friend err_t tcp_recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
		friend void  timeout_cb(void *arg);

		// a reference to the application logger
		tools::Logger* const _logger;

		// current state of the forwarder
		State _state;

		// 
		const Endpoint _endpoint;

		// tcp no delay mode is enabled
		const bool _tcp_nodelay;

		// keep alive delay
		const int _keepalive;

		// local endpoint acting as a server. 
		Socket _local_server;
		
		// local endpoint acting as a client.
		struct ::tcp_pcb* _local_client;
		
		// connect timeout
		bool _connect_timeout;

		// forward flush timeout
		bool _fflush_timeout;

		// reply flush timeout
		bool _rflush_timeout;

		// reply and forward queues
		WinsOutputQueue _reply_queue;
		LwipOutputQueue _forward_queue;

		// bytes in transit
		size_t _forwarded_bytes;
	};

}