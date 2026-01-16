/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "SyncDisconnect.h"
#include <memory>

namespace ui {
	SyncDisconnect::SyncDisconnect(HWND hwnd, fw::FirewallClient& portal_client, fw::FirewallTunnel* tunnel) :
		SyncProc(hwnd, AsyncMessage::DisconnectedEvent.get()),
		_portal_client(portal_client),
		_tunnel(tunnel)
	{
		DEBUG_CTOR(_logger);
	}


	SyncDisconnect::~SyncDisconnect()
	{
		DEBUG_DTOR(_logger);
	}


	bool SyncDisconnect::procedure()
	{
		DEBUG_ENTER(_logger);
		bool stopped = false;

		if (_portal_client.is_authenticated()) {
			LOG_DEBUG(_logger, "logout from portal 0x%012Ix", PTR_VAL(std::addressof(_portal_client)));

			// Logs out from the firewall portal and waits for the firewall to close the tunnel.
			if (_portal_client.logout()) {
				LOG_DEBUG(_logger, "wait end of tunnel 0x%012Ix", PTR_VAL(std::addressof(_tunnel)));
				if (_tunnel)
					stopped = _tunnel->wait(5 * 1000);
				else
					stopped = true;
			}

			if (!stopped && _tunnel) {
				// Logout failed, forcefully shut down the tunnel.
				LOG_DEBUG(_logger, "terminate tunnel 0x%012Ix", PTR_VAL(std::addressof(_tunnel)));
				_tunnel->terminate();

				// Wait the end of the tunnel.  15 seconds should be enough to
				// let LwIp close the PPP interface.
				bool wait = true;
				for (int i = 0; i < 5 && wait; i++) {
					if (!_tunnel->wait(5 * 1000)) {
						if (i == 0)
							_logger->info(">> waiting for tunnel to shutdown...");
					}
					else {
						wait = false;
					}
				}

				if (wait) {
					_logger->error("ERROR: unable to shutdown the tunnel");
				}
			}
		}

		// Make sure the socket is disconnected from the portal.
		const utl::mbed_err rc = _portal_client.shutdown();
		if (rc)
			_logger->error("ERROR: close notify error (%d)", rc);

		return stopped;
	}

	const char* SyncDisconnect::__class__ = "SyncDisconnect";
}
