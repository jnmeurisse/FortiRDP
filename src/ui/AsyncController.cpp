/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "AsyncController.h"

#include "ui/SyncConnect.h"
#include "ui/SyncWaitTunnel.h"
#include "ui/SyncDisconnect.h"
#include "ui/SyncWaitTask.h"
#include "tools/Logger.h"
#include "tools/Mutex.h"
#include "tools/SysUtil.h"
#include <mbedtls/pk.h>


namespace ui {

	AsyncController::AsyncController(HWND hwnd) :
		tools::Thread(),
		_logger(tools::Logger::get_logger()),
		_action(NONE),
		_mutex(),
		_requestEvent(false),
		_readyEvent(false),
		_hwnd(hwnd),
		_ca_crt(),
		_user_crt(),
		_auth_method(fw::AuthMethod::BASIC)
	{
		DEBUG_CTOR(_logger, "AsyncController");
		start();
	}


	AsyncController::~AsyncController()
	{
		DEBUG_DTOR(_logger, "AsyncController");
	}


	bool AsyncController::load_ca_crt(const tools::Path& filename)
	{
		DEBUG_ENTER(_logger, "PortalClient", "load_ca_crt");
		bool init_status = true;

		if (!_ca_crt) {
			const std::string crt_filename = tools::wstr2str(filename.to_string());
			const std::string compacted = tools::wstr2str(filename.compact(40));

			_ca_crt = std::make_unique<tools::X509Crt>();

			if (tools::file_exists(filename)) {
				tools::mbed_err rc = _ca_crt->load(crt_filename.c_str());

				if (rc != 0) {
					_logger->info("WARNING: failed to load CA cert file %s ", compacted.c_str());
					_logger->info("%s", tools::mbed_errmsg(rc).c_str());
					init_status = false;
				}
				else {
					_logger->info(">> CA cert loaded from file '%s'", compacted.c_str());
					init_status = true;
				}
			}
			else {
				_logger->error("ERROR: can't find CA cert file %s", compacted.c_str());
				init_status = false;
			}
		}

		return init_status;
	}


	bool AsyncController::load_user_crt(const tools::Path& filename, ask_crt_passcode_fn ask_passcode)
	{
		DEBUG_ENTER(_logger, "PortalClient", "load_user_crt");
		bool init_status = true;

		if (!_user_crt) {
			const std::string crt_filename = tools::wstr2str(filename.to_string());
			const std::string compacted = tools::wstr2str(filename.compact(40));

			_user_crt = std::make_unique<tools::UserCrt>();

			if (tools::file_exists(filename)) {
				// Load the user certificate
				tools::mbed_err rc = _user_crt->crt.load(crt_filename.c_str());

				if (rc != 0) {
					_logger->error("ERROR: failed to load user cert file %s ", compacted.c_str());
					_logger->info("%s", tools::mbed_errmsg(rc).c_str());
					init_status = false;
				}
				else {
					// try to load the private key without a password
					tools::mbed_err rc = _user_crt->pk.load(crt_filename.c_str(), nullptr);

					if (rc == MBEDTLS_ERR_PK_PASSWORD_REQUIRED) {
						std::string passcode;

						if (ask_passcode(passcode)) {
							rc = _user_crt->pk.load(crt_filename.c_str(), nullptr);
						}
					}
					if (rc) {
						_logger->error("ERROR: can't load private key from file %s", compacted.c_str());
						init_status = false;
					}
				}
			}
			else {
				_logger->error("ERROR: can't find user cert file %s", compacted.c_str());
				init_status = false;
			}
		}

		return init_status;
	}


	void AsyncController::set_auth_method(fw::AuthMethod auth_method)
	{
		_auth_method = auth_method;
	}


	bool AsyncController::connect(const net::Endpoint& firewall_endpoint, const std::string& realm)
	{
		if (_logger->is_debug_enabled()) {
			_logger->debug("... %x enter AsyncController::connect ep=%s realm=%s",
				(uintptr_t)this,
				firewall_endpoint.to_string().c_str(),
				realm.c_str());
		}

		// Start the async connect procedure
		_portal = std::make_unique<fw::PortalClient>(firewall_endpoint, realm);
		_portal->set_ca_crt(_ca_crt->get_crt());

		if (_auth_method == fw::AuthMethod::CERTIFICATE && _user_crt)
			_portal->set_user_crt(_user_crt->crt.get_crt(), _user_crt->pk.get_pk());

		request_action(AsyncController::CONNECT);

		return _portal != nullptr;
	}


	bool AsyncController::create_tunnel(const net::Endpoint& host_endpoint, int local_port,
		bool multi_clients, bool tcp_nodelay)
	{
		if (_logger->is_debug_enabled()) {
			_logger->debug("... %x enter AsyncController::create_tunnel ep=%s",
				(uintptr_t)this,
				host_endpoint.to_string().c_str());
		}
		_tunnel.reset();

		if (_portal != nullptr) {
			/* Define the local end point as localhost which force using an IPv4 address.
			When local port is 0, the system automatically find a valid value. */
			const std::string localhost = "127.0.0.1";
			const net::Endpoint local_endpoint(localhost, local_port);
			const net::tunneler_config config = { tcp_nodelay, multi_clients ? 32 : 1 };

			// Create a ssl tunnel from this host to the firewall and assign it to local pointer
			_tunnel.reset(_portal->create_tunnel(local_endpoint, host_endpoint, config));
			request_action(AsyncController::TUNNEL);
		}

		return _tunnel != nullptr;
	}


	bool AsyncController::start_task(const tools::TaskInfo& task_info, bool monitor)
	{
		DEBUG_ENTER(_logger, "AsyncController", "start_task");

		bool started = false;

		if (_tunnel) {
			const net::Endpoint& endpoint = _tunnel->local_endpoint();

			tools::strimap vars;
			using pairofstr = std::pair<const std::string, std::string>;
			vars.insert(pairofstr("host", endpoint.hostname()));
			vars.insert(pairofstr("port", std::to_string(endpoint.port())));

			_task = std::make_unique<tools::Task>(task_info.path());
			for (unsigned int i = 0; i < task_info.params().size(); i++)
				_task->add_parameter(tools::substvar(task_info.params()[i], vars));

			started = _task->start();

			if (started && monitor) {
				// Request the active loop to monitor the task
				request_action(AsyncController::MONITOR_TASK);
			}

		}
		return started;
	}


	bool AsyncController::disconnect()
	{
		DEBUG_ENTER(_logger, "AsyncController", "disconnect");

		request_action(AsyncController::DISCONNECT);
		return _tunnel != nullptr;
	}



	bool AsyncController::terminate()
	{
		DEBUG_ENTER(_logger, "AsyncController", "terminate");

		request_action(AsyncController::TERMINATE);
		return true;
	}


	void AsyncController::request_action(ControllerAction action)
	{
		_logger->debug("... %x enter AsyncController::request_action action=%s",
			(uintptr_t)this,
			action_name(action));

		// Only one thread can request an action
		tools::Mutex::Lock lock(_mutex);

		// Wait that the controller thread is ready
		_logger->debug(".... %x AsyncController::request_action wait for action=%s",
			(uintptr_t)this,
			action_name(action));
		_readyEvent.wait();

		// define the action and wakeup the thread
		_logger->debug(".... %x AsyncController::request_action set event for action=%s",
			(uintptr_t)this,
			action_name(action));
		_action = action;
		if (!_requestEvent.set()) {
			_logger->error("ERROR: %x AsyncController::request_action set event error=%x",
				(uintptr_t)this,
				::GetLastError());
		}
	}

	const char* AsyncController::action_name(ControllerAction action) const
	{
		static const char* const actions_names[] = {
			"Connect",
			"Tunnel",
			"Disconnect",
			"Ping",
			"Task",
			"Terminate"
		};

		return actions_names[action];
	}


	unsigned int AsyncController::run()
	{
		bool terminated = false;	// a flag set to true to terminate this thread
		bool wait_eot = false;		// a flag set to true to monitor the end of a task

		HANDLE hEvents[2] = { _requestEvent.get_handle(), NULL };

		while (!terminated) {
			std::unique_ptr<SyncProc> procedure;

			// We are ready to accept a new event
			if (!_readyEvent.set()) {
				_logger->error("ERROR: %x AsyncController::run set event error=%x",
					(uintptr_t)this,
					::GetLastError());
			}

			/* The function waits for an action and optionally for the end of a task monitored at a previous
			 * iteration of the wait loop. We set eventCount to 2 if the AsyncProcedure monitors
			 * the end of a task.
			*/
			const int eventCount = wait_eot ? 2 : 1;
			DWORD event = WaitForMultipleObjects(eventCount, hEvents, false, INFINITE);

			_logger->debug("... %x enter AsyncController::run event=%x", (uintptr_t)this, event);

			// Wait that a new action is requested or that a task ended.
			switch (event) {
			case (WAIT_OBJECT_0 + 0):
				_logger->debug("... %x AsyncController::run action=%s",
					(uintptr_t)this,
					action_name(_action));

				// Create a procedure for the requested action.
				switch (_action)
				{
				case AsyncController::CONNECT:
					procedure = std::make_unique<SyncConnect>(_hwnd, _auth_method, _portal.get());
					break;

				case AsyncController::TUNNEL:
					procedure = std::make_unique<SyncWaitTunnel>(_hwnd, _tunnel.get());
					break;

				case AsyncController::DISCONNECT:
					procedure = std::make_unique<SyncDisconnect>(_hwnd, _portal.get(), _tunnel.get());
					break;

				case AsyncController::MONITOR_TASK:
					wait_eot = true;
					hEvents[1] = _task->get_handle();
					break;

				case AsyncController::TERMINATE:
					terminated = true;
					break;

				default:
					break;
				}
				break;

			case (WAIT_OBJECT_0 + 1):
				wait_eot = false;
				procedure = std::make_unique<SyncWaitTask>(_hwnd, _task.get());
				break;

			case WAIT_FAILED:
				_logger->error("ERROR: %x AsyncController::run wait failed error=%x",
					(uintptr_t)this,
					::GetLastError());
				terminated = true;
				break;

			default:
				break;
			}


			if (procedure != nullptr) {
				// Execute the procedure
				try {
					procedure->run();
				}
				catch (const std::exception& e) {
					_logger->error("ERROR: %x AsyncController::run failure exception=%s",
						(uintptr_t)this,
						e.what());
					terminated = true;

					
					throw;
				}
			}
		}

		_logger->debug("... %x leave AsyncController::run", (uintptr_t)this);

		return 0;
	}

}
