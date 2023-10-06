/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/

#include <stdlib.h>
#include <winsock2.h>
#include <Ws2ipdef.h>
#include <Windows.h>

#include "mbedtls/net_sockets.h"
#include "mbedccl/netctx.h"


netctx* netctx_alloc()
{
	mbedtls_net_context* const net_context = malloc(sizeof(mbedtls_net_context));

	if (net_context)
		mbedtls_net_init(net_context);

	return (netctx *)net_context;
}


void netctx_free(netctx* ctx)
{
	mbedtls_net_context* const net_context = (mbedtls_net_context*)ctx;

	if (net_context)
		// free the allocated memory
		free(net_context);
}


mbed_err netctx_close(netctx* ctx)
{
	mbed_err rc = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	mbedtls_net_context* const net_context = (mbedtls_net_context*)ctx;

	if (net_context) {
		mbedtls_net_close(net_context);
		rc = 0;
	}

	return rc;
}


mbed_err netctx_shutdown(netctx* ctx)
{
	mbed_err rc = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	mbedtls_net_context* const net_context = (mbedtls_net_context*)ctx;

	if (net_context) {
		mbedtls_net_free(net_context);
		rc = 0;
	}

	return rc;
}


int netctx_getfd(netctx* ctx)
{
	mbedtls_net_context* const net_context = (mbedtls_net_context*)ctx;
	return net_context ? net_context->fd : -1;
}


mbed_err netctx_connect(netctx* ctx, const char* hostname, const char* port, netctx_protocol protocol)
{
	mbed_err rc = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	mbedtls_net_context* const net_context = (mbedtls_net_context*)ctx;

	//TODO: https://github.com/Mbed-TLS/mbedtls/issues/8027

	if (net_context && hostname && port) {
		rc = mbedtls_net_connect(net_context, hostname, port, protocol);
		
		if (rc) { 
			// mbedtls_net_connect should call mbedtls_net_close instead of close in 
			// case of error.
			net_context->fd = -1;
		}
	}

	return rc;
}


mbed_err netctx_bind(netctx* ctx, const char* hostname, const char* port, netctx_protocol protocol)
{
	mbed_err rc = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	mbedtls_net_context* const net_context = (mbedtls_net_context*)ctx;

	if (net_context && hostname && port)
		rc = mbedtls_net_bind(net_context, hostname, port, protocol);

	return rc;
}


mbed_err netctx_accept(netctx* bind_ctx, netctx* accepting_ctx)
{
	mbed_err rc = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	mbedtls_net_context* const bind_net_context = (mbedtls_net_context*)bind_ctx;
	mbedtls_net_context* const client_net_context = (mbedtls_net_context*)accepting_ctx;

	if (bind_net_context && (netctx_getfd(accepting_ctx) != -1))
		rc = mbedtls_net_accept(bind_net_context, client_net_context, 0, 0, 0);

	return rc;
}


mbed_err netctx_set_blocking(netctx* ctx, int blocking)
{
	int rc = INVALID_SOCKET;
	mbedtls_net_context* const net_context = (mbedtls_net_context*)ctx;

	if (net_context) {
		rc = blocking
			? mbedtls_net_set_block(net_context)
			: mbedtls_net_set_nonblock(net_context);
	}

	return rc ? MBEDTLS_ERR_NET_INVALID_CONTEXT : 0;
}


mbed_err netctx_set_nodelay(netctx* ctx, int nodelay)
{
	int rc = INVALID_SOCKET;
	mbedtls_net_context* const net_context = (mbedtls_net_context*)ctx;

	if (net_context) {
		const int tcp_nodelay = nodelay ? 1 : 0;
		rc = setsockopt(
				net_context->fd, 
				IPPROTO_TCP, 
				TCP_NODELAY, 
				(const char *)&tcp_nodelay, 
				sizeof(tcp_nodelay));
	}

	return rc ? MBEDTLS_ERR_NET_INVALID_CONTEXT : 0;
}


int netctx_get_port(netctx* ctx)
{
	int port = -1;
	mbedtls_net_context* const net_context = (mbedtls_net_context*)ctx;

	if (net_context) {
		struct sockaddr_storage sock_addr;
		int len = sizeof(sock_addr);

		if (getsockname(net_context->fd, (struct sockaddr *)&sock_addr, &len) == 0) {
			if (sock_addr.ss_family == AF_INET) {
				struct sockaddr_in *addr4 = (struct sockaddr_in *) &sock_addr;
				port = addr4->sin_port;
			}
			else if (sock_addr.ss_family == AF_INET6) {
				struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) &sock_addr;
				port = addr6->sin6_port;
			}
		}
	}

	return port;
}


netctx_poll_status netctx_poll(netctx* ctx, netctx_poll_mode mode, uint32_t timeout)
{
	netctx_poll_status status = { NETCTX_POLL_ERROR, 0, MBEDTLS_ERR_SSL_BAD_INPUT_DATA };
	mbedtls_net_context* const net_context = (mbedtls_net_context*)ctx;
	uint32_t rw = 0;

	// build the read/write flag
	if (mode.read == 1)
		rw = MBEDTLS_NET_POLL_READ;
	if (mode.write == 1)
		rw |= MBEDTLS_NET_POLL_WRITE;

	if (net_context && rw) {
		// wait to be ready for read or write
		const int rc = mbedtls_net_poll(net_context, rw, timeout);
		if (rc < 0) {
			// an error has occurred
			status.errnum = rc;
		}
		else if (rc == 0) {
			// wait timed out
			status.status_code = NETCTX_POLL_ERROR;
			status.errnum = MBEDTLS_ERR_SSL_TIMEOUT;
		}
		else {
			// ready for read and/or write
			status.status_code = NETCTX_POLL_OK;
			status.errnum = 0;
			status.event.read = rc & MBEDTLS_NET_POLL_READ;
			status.event.write = rc & MBEDTLS_NET_POLL_WRITE;
		}
	}

	return status;
}


netctx_snd_status netctx_send(netctx* ctx, const unsigned char* buf, size_t len)
{
	netctx_snd_status status = { NETCTX_SND_ERROR, MBEDTLS_ERR_SSL_BAD_INPUT_DATA, 0 };
	mbedtls_net_context* const net_context = (mbedtls_net_context*)ctx;

	if (net_context) {
		int rc = mbedtls_net_send(net_context, buf, len);

		if (rc > 0) {
			status.status_code = NETCTX_SND_OK;
			status.errnum = 0;
			status.sbytes = rc;
		}
		else if (rc == MBEDTLS_ERR_SSL_WANT_WRITE) {
			status.status_code = NETCTX_SND_RETRY;
			status.errnum = (mbed_err)rc;
		}
		else {
			status.status_code = NETCTX_SND_ERROR;
			status.errnum = (mbed_err)rc;
		}
	}

	return status;
}


netctx_rcv_status netctx_recv(netctx* ctx, unsigned char* buf, size_t len)
{
	netctx_rcv_status status = { NETCTX_RCV_ERROR, MBEDTLS_ERR_SSL_BAD_INPUT_DATA, 0 };
	mbedtls_net_context* const net_context = (mbedtls_net_context*)ctx;

	if (net_context) {
		int rc = mbedtls_net_recv(net_context, buf, len);

		if (rc > 0) {
			status.status_code = NETCTX_RCV_OK;
			status.errnum = 0;
			status.rbytes = rc;
		}
		else if (rc == 0) {
			status.status_code = NETCTX_RCV_EOF;
			status.errnum = 0;
		}
		else if (rc == MBEDTLS_ERR_SSL_WANT_READ) {
			status.status_code = NETCTX_RCV_RETRY;
			status.errnum = (mbed_err)rc;
		}
		else {
			status.status_code = NETCTX_RCV_ERROR;
			status.errnum = (mbed_err)rc;
		}
	}

	return status;
}

