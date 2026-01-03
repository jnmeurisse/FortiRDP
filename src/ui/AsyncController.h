/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include "fw/FirewallClient.h"
#include "fw/FirewallTunnel.h"
#include "net/TlsConfig.h"
#include "net/Endpoint.h"
#include "util/Event.h"
#include "util/Logger.h"
#include "util/Mutex.h"
#include "util/Path.h"
#include "util/Thread.h"
#include "util/TaskInfo.h"
#include "util/Task.h"
#include "util/UserCrt.h"
#include "util/X509Crt.h"


namespace ui {

	// Callback definition
	using ask_crt_passcode_fn = std::function<bool(std::string&)>;


	/**
	* The AsyncController is a singleton class. This controller is responsible to execute all
	* blocking operations in a separated thread. A message is sent to the window specified by
	* the hwnd parameter when the blocking operation is finished. A blocking operation is
	* implemented in a SyncProcedure sub class.
	*/
	class AsyncController final : public aux::Thread
	{
	public:
		explicit AsyncController(HWND hwnd);
		virtual ~AsyncController() override;

		/**
		 * Initialize the CA certificate chain from the given file.
		 * 
		*/
		bool load_ca_crt(const aux::Path& filename);

		/**
		 * Initialize the user certificate.
		*/
		bool load_user_crt(const aux::Path& filename, ask_crt_passcode_fn ask_passcode);

		/**
		 * Configure the authentication method.
		*/
		void set_auth_method(fw::AuthMethod auth_method);

		/**
		 * Connects this controller to the firewall
		 *
		 * The method creates a portal client and connects it to the firewall.
		*/
		bool connect(const net::Endpoint& firewall_endpoint, const std::string& realm);

		/**
		 * Creates a tunnel with the firewall.
		*/
		bool create_tunnel(const net::Endpoint& remote_endpoint, uint16_t local_port, bool multi_clients, bool tcp_nodelay);

		/**
		 * Starts an external task. 
		 *
		 * The monitor flag indicates that the  controller is monitoring the 
		 * completion of the external task.
		*/
		bool start_task(const aux::TaskInfo& task_info, bool monitor);

		/**
		 * Disconnects the controller from the firewall.
		*/
		bool disconnect();

		/**
		 * Terminates this controller.
		*/
		bool terminate();

		/**
		 * Returns a reference to the firewall client.
		 * 
		 * The method returns a null pointer if the client is not yet allocated.
		*/
		inline fw::FirewallClient* portal_client() const { return _portal_client.get(); };

		/**
		 * Returns a reference to the tunnel.
		 * 
		 * The method returns a null pointer if the tunnel is not yet allocated.
		*/
		inline fw::FirewallTunnel* tunnel() const { return _tunnel.get(); }

	private:
		// The class name.
		static const char* __class__;

		// The application logger.
		aux::Logger* const _logger;

		// The list of actions performed by the AsyncController in a background thread.
		enum ControllerAction {
			NONE,
			CONNECT,		// connect to the firewall portal
			TUNNEL,			// establish a tunnel with the firewall
			DISCONNECT,		// close the tunnel and disconnecting from the firewall portal  
			MONITOR_TASK,	// monitor the external task
			TERMINATE		// terminate this controller
		};

		// An action to be executed by the controller.
		volatile ControllerAction _action;

		// A mutex to serialize execution of actions.
		aux::Mutex _mutex;

		// An event set to execute an action.
		aux::Event _requestEvent;
		aux::Event _readyEvent;

		// Tthe recipient window of the user event message sent at completion of an action.
		const HWND _hwnd;

		// The CA certificate chains.
		aux::X509crtPtr _ca_crt;

		// The user certificate.
		aux::UserCrtPtr _user_crt;

		// The authentication method.
		fw::AuthMethod _auth_method;

		// The TLS configuration.
		net::TlsConfig _tls_config;

		std::unique_ptr<fw::FirewallClient> _portal_client;
		std::unique_ptr<fw::FirewallTunnel> _tunnel;
		std::unique_ptr<aux::Task> _task;

		/**
		 * Requests an action and wake up the thread.
		*/
		void request_action(ControllerAction task);

		/**
		 * Converts and action to a string.
		*/
		const char* action_name(ControllerAction action) const;

		/**
		 * The thread procedure.
		*/
		virtual unsigned int run() override;
	};

}
