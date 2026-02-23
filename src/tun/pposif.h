/*!
* This file is part of FortiRDP
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#ifndef TUNOSSL_H
#define TUNOSSL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lwip/netif.h>
#include <lwip/err.h>
#include <lwip/pbuf.h>

err_t pppif_init(struct netif* netif);

#ifdef __cplusplus
}
#endif

#endif