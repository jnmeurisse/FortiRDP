/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "net/Socket.h"
#include "net/Listener.h"
#include "net/Endpoint.h"
#include "net/WinsOutputQueue.h"
#include "net/LwipOutputQueue.h"
#include "tools/Logger.h"
#include "lwip/tcp.h"


namespace net {

	class PortForwarder final {
	public:
		explicit PortForwarder(bool tcp_nodelay, int keepalive);
		~PortForwarder();

		/*
		*/
		bool connect(Listener& listener, const Endpoint& remote_endpoint);
		
		/*
		*/
		void disconnect();

		/*
		*/
		void abort();

		/*
		*/
		inline bool connected() const { return _state == State::CONNECTED; }

		/*
		*/
		inline bool connecting() const { return _state == State::CONNECTING; }

		/*
		*/
		inline bool ctimeout() const { return _state == State::CONNECTING && _connect_timeout; }


		/*
		*/
		inline bool disconnecting() const { return _state == State::DISCONNECTING; }

		/*
		*/
		inline bool failed() const { return _state == State::FAILED; }

		/*
		*/
		inline bool disconnected() const { return _state == State::DISCONNECTED; }

		/*
		*/
		inline bool can_receive() const { return !_forward_queue.full(); }

		/*
		*/
		inline bool must_forward() const { return !_forward_queue.empty() && tcp_sndbuf(_local_client) > 0; }

		/*
		*/
		inline bool must_reply() const { return _reply_queue.len() > 0; }

		/*
		*/
		inline bool can_rflush() const { return  _local_server.connected(); }

		/*
		*/
		inline bool can_fflush() const { return _local_client != nullptr; }

		/*
		*/
		inline int get_fd() const { return _local_server.get_fd(); }
		

		/*
		*/
		bool recv();


		/*
		*/
		bool forward();
		
		/*
		*/
		bool reply();

		/*
		*/
		void fflush();

		/*
		*/
		void rflush();

	private:
		// Port forwarder states
		enum State {
			READY,					// The port forwarder is allocated and ready 
			FAILED,
			CONNECTING,				// 
			CONNECTED,
			DISCONNECTING,
			DISCONNECTED
		};

		friend err_t tcp_connected_cb(void *arg, struct tcp_pcb *tpcb, err_t err);
		friend void  tcp_err_cb(void *arg, err_t err);
		friend err_t tcp_sent_cb(void *arg, struct tcp_pcb *tpcb, u16_t len);
		friend err_t tcp_recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
		friend void  timeout_cb(void *arg);


		// a reference to the application logger
		tools::Logger* const _logger;

		// current state of the port forwarder
		State _state;

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

		// reply flush timer
		bool _rflush_timeout;


		WinsOutputQueue _reply_queue;
		LwipOutputQueue _forward_queue;
		

		// bytes in transit
		size_t _forwarded_bytes;
	};

}