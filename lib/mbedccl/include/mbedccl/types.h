s/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/

#pragma once
typedef struct netctx netctx;
typedef struct sslcfg sslcfg;
typedef struct sslctx sslctx;
typedef struct x509_crt x509_crt;

typedef enum netctx_protocol {
	NETCTX_PROTO_TCP = 0,
	NETCTX_PROTO_UDP = 1
} netctx_protocol;



