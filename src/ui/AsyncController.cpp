/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "AsyncController.h"
#include "SyncConnect.h"
#include "SyncWaitTunnel.h"
#include "SyncDisconnect.h"
#include "SyncWaitTask.h"
#include "tools/Mutex.h"


AsyncController::AsyncController(HWND hwnd):
	tools::Thread(),
	_logger(tools::Logger::get_logger()),
	_hwnd(hwnd),
	_mutex(),
	_requestEvent(false),
	_readyEvent(false)
{
	DEBUG_CTOR(_logger, "AsyncController");
	start();
}


AsyncController::~AsyncController()
{
	DEBUG_DTOR(_logger, "AsyncController");
}


bool AsyncController::connect(const net::Endpoint& firewall_endpoint, const fw::CertFiles& cert_files)
{
	if (_logger->is_debug_enabled()) {
		_logger->debug("... %x enter AsyncController::connect ep=%s",
			(uintptr_t)this,
			firewall_endpoint.to_string().c_str());
	}

	// Start the async connect procedure
	_portal = std::make_unique<fw::PortalClient>(firewall_endpoint, cert_files);
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
			case (WAIT_OBJECT_0 + 0) : 
				_logger->debug("... %x AsyncController::run action=%s",
					(uintptr_t)this,
					action_name(_action));

				// Create a procedure for the requested action.
				switch (_action)
				{
				case AsyncController::CONNECT:
					procedure = std::make_unique<SyncConnect>(_hwnd, _portal.get());
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

			case (WAIT_OBJECT_0 + 1) : 
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
