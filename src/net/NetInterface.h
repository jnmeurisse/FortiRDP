/*!
* This file is part of FortiRDP
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <lwip/err.h>
#include <lwip/netif.h>
#include <lwip/pbuf.h>

namespace net {
	class NetInterface
	{
	public:
		NetInterface();
		void init();

		//virtual void on_link_up();
		//virtual void on_link_down();
	
		virtual void on_ouput(struct pbuf *p) = 0;

	private:
		struct netif _netif;
	};
}