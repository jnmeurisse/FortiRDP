/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "SyncDisconnect.h"

namespace ui {
	SyncDisconnect::SyncDisconnect(HWND hwnd, fw::PortalClient* portal, net::Tunneler* tunnel) :
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
		bool status = false;


		if (_portal && _portal->is_authenticated())
			_logger->info(">> disconnecting");

		// terminate the tunnel
		if (_tunnel) {
			_logger->debug("... terminate tunnel %x", (uintptr_t)_tunnel);
			_tunnel->terminate();

			_logger->debug("... wait end of tunnel %x", (uintptr_t)_tunnel);

			// wait the end of the tunnel.  20 seconds should be enough to
			// let LwIp close the ppp interface.  If not, we will forcibly close
			// the socket without executing a proper shutdown.
			bool wait = true;
			for (int i = 0; i < 4 && wait; i++) {
				if (!_tunnel->wait(5 * 1000)) {
					if (i == 0)
						_logger->info(">> waiting for tunnel to shutdown...");
				}
				else {
					wait = false;
				}
			}

			if (wait)
				_logger->error("ERROR: unable to shutdown the tunnel");

		}

		// Close the TLS socket
		if (_portal && _portal->is_authenticated()) {
			_logger->debug("... logoff from portal %x", (uintptr_t)_portal);
			_portal->logoff();
		}

		if (_portal && _portal->is_connected()) {
			_logger->debug("... close portal %x", (uintptr_t)_portal);
			_portal->close();

			status = true;
		}

		return status;
	}

}
