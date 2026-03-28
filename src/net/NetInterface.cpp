/*!
* This file is part of FortiRDP
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "NetInterface.h"
#include <lwip/pbuf.h>
#include <lwip/ip_addr.h>


namespace net {

	err_t netif_linkoutput_cb(struct netif* netif, struct pbuf* p);


	NetInterface::NetInterface() :
		_netif()
	{
		_netif.state = this;

	}


	void NetInterface::init()
	{
		_netif.linkoutput = netif_linkoutput_cb;
	}


	err_t netif_linkoutput_cb(struct netif* netif, struct pbuf* p)
	{
		NetInterface* nif = static_cast<NetInterface *>(netif->state);
		nif->ouput(p);
	}


}