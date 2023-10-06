/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "mbedccl/error.h"

typedef enum netctx_protocol {
	NETCTX_PROTO_TCP = 0,
	NETCTX_PROTO_UDP = 1
} netctx_protocol;


typedef struct netctx_poll_mode {
	unsigned int read:1;
	unsigned int write:1;
} netctx_poll_mode;


typedef enum netctx_rcv_status_code {
	NETCTX_RCV_OK    = 0,
	NETCTX_RCV_RETRY = 1,
	NETCTX_RCV_ERROR = 2,
	NETCTX_RCV_EOF   = 3
} netctx_rcv_status_code;


typedef enum netctx_snd_status_code {
	NETCTX_SND_OK    = 0,
	NETCTX_SND_RETRY = 1,
	NETCTX_SND_ERROR = 2
} netctx_snd_status_code;


typedef enum sslctx_hdk_status_code {
	SSLCTX_HDK_OK         = 0,
	SSLCTX_HDK_WAIT_IO    = 1,
	SSLCTX_HDK_WAIT_ASYNC = 2,
	SSLCTX_HDK_ERROR      = 3
} netctx_hdk_status_code;

typedef enum netctx_poll_status_code {
	NETCTX_POLL_OK = 0,
	NETCTX_POLL_TIMEOUT = 1,
	NETCTX_POLL_ERROR = 2
} netctx_poll_status_code;

typedef struct netctx_poll_status {
	netctx_poll_status_code  status_code;
	netctx_poll_mode event;
	mbed_err errnum;
} netctx_poll_status;

typedef struct netctx_rcv_status {
	netctx_rcv_status_code status_code;
	mbed_err errnum;
	size_t rbytes;
} netctx_rcv_status;


typedef struct netctx_snd_status {
	netctx_snd_status_code status_code;
	mbed_err errnum;
	size_t sbytes;
} netctx_snd_status;


typedef struct sslctx_handshake_status {
	netctx_hdk_status_code status_code;
	mbed_err errnum;
} sslctx_handshake_status;
