/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "SyncDisconnect.h"

namespace ui {
	SyncDisconnect::SyncDisconnect(HWND hwnd, fw::PortalClient& portal, fw::FirewallTunnel& tunnel) :
		SyncProc(hwnd, AsyncMessage::DisconnectedEvent),
		_portal(portal),
		_tunnel(tunnel)
	{
		DEBUG_CTOR(_logger, "SyncDisconnect");
	}


	SyncDisconnect::~SyncDisconnect()
	{
		DEBUG_DTOR(_logger, "SyncDisconnect");
	}


	bool SyncDisconnect::procedure()
	{
		DEBUG_ENTER(_logger, "SyncDisconnect", "procedure");
		bool stopped = false;

		if (_portal.is_authenticated()) {
			_logger->debug("... logout from portal %x", (uintptr_t)&_portal);

			// Logs out from the firewall portal and waits for the firewall to close the tunnel.
			if (_portal.logoff() == fw::portal_err::NONE) {
				_logger->debug("... wait end of tunnel %x", (uintptr_t)&_tunnel);
				stopped = _tunnel.wait(5 * 1000);
			}

			if (!stopped) {
				// Logoff failed, forcefully shut down the tunnel.
				_logger->debug("... terminate tunnel %x", (uintptr_t)&_tunnel);
				_tunnel.terminate();

				// Wait the end of the tunnel.  15 seconds should be enough to
				// let LwIp close the ppp interface.
				bool wait = true;
				for (int i = 0; i < 5 && wait; i++) {
					if (!_tunnel.wait(5 * 1000)) {
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

		// Make sure the socket with the portal is disconnected.
		_portal.shutdown();

		return stopped;
	}

}
