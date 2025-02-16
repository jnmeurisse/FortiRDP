/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <Windows.h>
#include <functional>
#include <memory>
#include <string>
#include "fw/PortalClient.h"
#include "fw/FirewallTunnel.h"
#include "net/TlsConfig.h"
#include "net/Endpoint.h"
#include "tools/Event.h"
#include "tools/Logger.h"
#include "tools/Mutex.h"
#include "tools/Path.h"
#include "tools/Thread.h"
#include "tools/TaskInfo.h"
#include "tools/Task.h"
#include "tools/UserCrt.h"
#include "tools/X509Crt.h"


namespace ui {

	// Callback definition
	using ask_crt_passcode_fn = std::function<bool(std::string&)>;


	/**
	* The AsyncController is a singleton class. This controller is responsible to execute all
	* blocking operations in a separated thread. A message is sent to the window specified by
	* the hwnd parameter when the blocking operation is finished. A blocking operation is
	* implemented in a SyncProcedure sub class.
	*/
	class AsyncController final : public tools::Thread
	{
	public:
		explicit AsyncController(HWND hwnd);
		~AsyncController();

		/* Initialize the CA certificate chain from the given file.
		*  The root CA certificate issued to authenticate the server.
		*/
		bool load_ca_crt(const tools::Path& filename);

		/* Initialize the user certificate
		*/
		bool load_user_crt(const tools::Path& filename, ask_crt_passcode_fn ask_passcode);

		/* Configure the authentication method
		*/
		void set_auth_method(fw::AuthMethod auth_method);

		/* Connects the controller to the firewall. The methods creates a portal
		   client and connects it to the firewall.
		*/
		bool connect(const net::Endpoint& firewall_endpoint, const std::string& realm);

		/* Creates a tunnel with the firewall
		*/
		bool create_tunnel(const net::Endpoint& remote_endpoint, int local_port, bool multi_clients, bool tcp_nodelay);

		/* Starts the external task. The monitor flag indicates that the async
		   controller is monitoring the completion of the external task.
		*/
		bool start_task(const tools::TaskInfo& task_info, bool monitor);

		/* Disconnects the controller from the firewall.
		*/
		bool disconnect();

		/* Terminates this async controller
		*/
		bool terminate();

		/* Returns a reference to the PortalClient. The method returns a null pointer
		   if the portal is not yet allocated.
		*/
		inline fw::PortalClient* portal() const { return _portal.get(); };

		/* Returns a reference to the tunnel.  The method returns a null pointer
		   if the tunnel is not yet allocated.
		*/
		inline fw::FirewallTunnel* tunnel() const { return _tunnel.get(); }

	private:
		// - the application logger
		tools::Logger* const _logger;

		// - list of actions performed by the AsyncController in a background thread.
		enum ControllerAction {
			NONE,
			CONNECT,		// connect to the firewall portal
			TUNNEL,			// establish a tunnel with the firewall
			DISCONNECT,		// close the tunnel and disconnecting from the firewall portal  
			MONITOR_TASK,	// monitor the external task
			TERMINATE		// terminate this controller
		};

		// - action to execute by the controller
		volatile ControllerAction _action;

		// - Mutex to serialize execution of action
		tools::Mutex _mutex;

		// - event set to execute an action 
		tools::Event _requestEvent;
		tools::Event _readyEvent;

		// - the recipient window of the user event message sent at completion of an action
		const HWND _hwnd;

		// - CA certificate chains
		tools::X509crtPtr _ca_crt;

		// - user certificate
		tools::UserCrtPtr _user_crt;

		// - authentication method
		fw::AuthMethod _auth_method;

		// - the Tls configuration
		net::TlsConfig _tls_config;

		std::unique_ptr<fw::PortalClient> _portal;
		std::unique_ptr<fw::FirewallTunnel> _tunnel;
		std::unique_ptr<tools::Task> _task;

		/* Requests an action and wake up the thread
		*/
		void request_action(ControllerAction task);

		/* Convert action to string
		*/
		const char* action_name(ControllerAction action) const;

		/* The thread procedure
		*/
		virtual unsigned int run() override;
	};

}
