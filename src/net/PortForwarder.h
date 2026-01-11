/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <lwip/tcp.h>
#include "net/TcpSocket.h"
#include "net/Listener.h"
#include "net/Endpoint.h"
#include "net/OutputQueue.h"
#include "util/Logger.h"


namespace net {
	/**
	* PortForwarder: A class responsible for handling TCP port forwarding. It
	* accepts local client connections, resolves destination host names, forwards
	* data between local and remote endpoints, and manages connection states and
	* timeouts.
	*/

	class PortForwarder final {
	public:
		explicit PortForwarder(const net::Endpoint& endpoint, bool tcp_nodelay, int keepalive);
		~PortForwarder();

		/**
		 * Establishes a connection using the provided listener.
		 *
		 * This function initiates a connection from a local client to a remote endpoint
		 * through a listener. It handles accepting incoming connections, resolving the
		 * endpoint's host name to an IP address, and setting up the TCP client for
		 * communication. Additionally, it configures various TCP options like TCP_NODELAY
		 * and keep-alive, if specified.
		 *
		 * @param listener Reference to a `Listener` object that is bound to a local
		 *                 endpoint and waiting for incoming connections.
		 *
		 * @return bool Returns `true` if the connection setup is successfully initiated,
		 *              or `false` if an error occurs at any stage.
		 *
		 */
		bool connect(net::Listener& listener);
		
		/**
		 * Disconnects this forwarder from the server.
		 *
		 * This function initiates a graceful disconnection from the server, flushing
		 * queues, and transitioning to the DISCONNECTED state.
		*/
		void disconnect();

		/**
		 * Aborts the connection with the server.
		 *
		 * The function immediately terminates the connection by sending a TCP RST signal
		 * and clearing all associated queues.
		*/
		void abort();

		/**
		 * Returns true if this forwarder is connected.
		*/
		inline bool is_connected() const noexcept { return _state == State::CONNECTED; }

		/**
		 * Returns true if this forwarder is in the connecting phase.
		*/
		inline bool is_connecting() const noexcept { return _state == State::CONNECTING; }

		/**
		 * Returns true if the connection has timed out.
		*/
		inline bool has_connection_timed_out() const noexcept { return is_connecting() && _connect_timeout; }

		/**
		 * Returns true if this forwarder is in the disconnecting phase.
		*/
		inline bool is_disconnecting() const noexcept { return _state == State::DISCONNECTING; }

		/**
		 * Returns true if this forwarder was not able to connect.
		*/
		inline bool has_failed() const noexcept { return _state == State::FAILED; }

		/**
		 * Returns true if this forwarder is disconnected.
		*/
		inline bool is_disconnected() const noexcept { return _state == State::DISCONNECTED; }

		/**
		 * Returns true if this forwarder has space in the forward queue.
		*/
		inline bool can_receive_data() const noexcept { return !_forward_queue.is_full(); }

		/**
		 * Returns true if this forwarder has data in the forward queue and
		 * the TCP send buffer is not full or segments are queued but not yet sent.
		*/
		inline bool has_data_to_forward() const noexcept { 
			return has_pending_tcp_segment() || (!_forward_queue.is_empty() && !is_tcp_sndbuf_full());
		}

		/**
		 * Returns true if this forwarder has data in the reply queue.
		*/
		inline bool has_data_to_reply() const noexcept { return !_reply_queue.is_empty(); }

		/**
		 * Returns true if this forwarder can still flush the reply queue.
		*/
		inline bool can_flush_reply_queue() const noexcept { return  _local_server.is_connected(); }

		/**
		 * Returns true if this forwarder can still flush the forward queue.
		*/
		inline bool can_flush_forward_queue() const noexcept { return _local_client != nullptr; }

		/**
		 * Returns the underlying socket file descriptor.
		*/
		inline int get_fd() const noexcept { return _local_server.get_fd(); }
		
		/**
		 * Receives data from the local server and queues it for forwarding to
		 * the remote endpoint.
		*/
		bool recv();

		/**
		 * Sends queued data to the remote endpoint.
		*/
		bool forward();
		
		/**
		 * Sends queued replies back to the local client from the remote endpoint.
		*/
		bool reply();

		/**
		 * Flushes the forward queue.
		 *
		 * This function handles forward queue flushing during disconnection, ensuring all
		 * data is sent or cleared and the state is updated appropriately.
		*/
		void flush_forward_queue();

		/**
		 * Flushes the reply queue.
		 *
		 * This function handles reply queue flushing during disconnection, ensuring all
		 * replies are sent or cleared and the state is updated appropriately.
		*/
		void flush_reply_queue();

	private:
		// The class name
		static const char* __class__;

		// forwarder states
		enum class State {
			READY,					// The forwarder is allocated and ready 
			FAILED,					// The forwarder failed to connect
			CONNECTING,				// The forwarder is connecting
			CONNECTED,				// The forwarder is connected
			DISCONNECTING,			// The forwarder is disconnecting
			DISCONNECTED			// The forwarder is disconnected
		};

		// A callback when DNS resolution is completed.
		// This callback is responsible to create a TCP socket with
		// the remote host.  This socket is established through the secure
		// socket.
		friend void dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg);

		// A callback for successful TCP connection establishment.
		friend err_t tcp_connected_cb(void *arg, struct tcp_pcb *tpcb, err_t err);

		// A callback for handling TCP errors.
		friend void tcp_err_cb(void *arg, err_t err);

		// A callback for confirming data sent to the remote endpoint.  This callback
		// is responsible for updating the count of forwarded bytes.
		friend err_t tcp_sent_cb(void *arg, struct tcp_pcb *tpcb, u16_t len);

		// A callback for receiving data from the remote endpoint.
		friend err_t tcp_recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

		// Returns the available tcp buffer queue space for sending (in bytes).
		inline size_t tcp_snd_buffer_size() const noexcept { return tcp_sndbuf(_local_client); }

		// Returns true if the tcp buffer queue for sending is full.
		inline bool is_tcp_sndbuf_full() const noexcept { return tcp_snd_buffer_size() == 0; }

		// Returns true if the tcp queue has unsent segments.
		inline bool has_pending_tcp_segment() const noexcept { return _local_client->unsent != nullptr; }

		// A reference to the application logger.
		utl::Logger* const _logger;

		// The current state of the forwarder.
		State _state;

		// The end point this forwarder is connected to.
		const net::Endpoint _endpoint;

		// True if TCP no delay mode is enabled.
		const bool _tcp_nodelay;

		// The keep alive delay.
		const int _keepalive;

		// The local endpoint acting as a server.
		net::TcpSocket _local_server;
		
		// The local endpoint acting as a client.
		struct ::tcp_pcb* _local_client;
		
		// Indicates whether the connection timer has expired.
		bool _connect_timeout;

		// Indicates whether the forward flush timer has expired.
		bool _fflush_timeout;

		// Indicates whether the reply flush timer has expired.
		bool _rflush_timeout;

		// Queues used for forwarding and replying data between components:
		//   - Forward queue:  data sent from the local server through the forwarder 
		//                     to the remote endpoint
		//   - Reply queue:    data returned from the remote endpoint through
		//                     the forwarder to the local server
		//
		//   local server        forwarder        remote endpoint
		//                  -> forward queue -> 
		//                  <- reply queue   <-
		net::OutputQueue _reply_queue;
		net::OutputQueue _forward_queue;

		// Number of bytes in transit (sent to the remote endpoint)
		size_t _forwarded_bytes;
	};

}
