/*!
* This file is part of FortiRDP
*
* Copyright (C) 2025 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "NetContext.h"

#include <winsock2.h>
#include <Ws2ipdef.h>


mbedtls_net_context* netctx_alloc()
{
	mbedtls_net_context* const net_context = malloc(sizeof(mbedtls_net_context));

	if (net_context)
		mbedtls_net_init(net_context);

	return net_context;
}


void netctx_free(mbedtls_net_context* ctx)
{
	if (ctx)
		// free the allocated memory
		free(ctx);
}


void netctx_close(mbedtls_net_context* ctx)
{
	if (ctx)
		mbedtls_net_close(ctx);
}


void netctx_shutdown(mbedtls_net_context* ctx)
{
	if (ctx)
		mbedtls_net_free(ctx);
}


int netctx_getfd(mbedtls_net_context* ctx)
{
	return ctx ? ctx->fd : -1;
}


int netctx_connect(mbedtls_net_context* ctx, const char* hostname, const char* port, netctx_protocol protocol)
{
	int rc = MBEDTLS_ERR_NET_INVALID_CONTEXT;

	//TODO: https://github.com/Mbed-TLS/mbedtls/issues/8027

	if (ctx && hostname && port) {
		rc = mbedtls_net_connect(ctx, hostname, port, protocol);
		
		if (rc) { 
			// mbedtls_net_connect should call mbedtls_net_close instead of close in 
			// case of error.
			ctx->fd = -1;
		}
	}

	return rc;
}


int netctx_bind(mbedtls_net_context* ctx, const char* hostname, const char* port, netctx_protocol protocol)
{
	int rc = MBEDTLS_ERR_NET_INVALID_CONTEXT;

	if (ctx && hostname && port)
		rc = mbedtls_net_bind(ctx, hostname, port, protocol);

	return rc;
}


int netctx_accept(mbedtls_net_context* bind_ctx, mbedtls_net_context* accepting_ctx)
{
	int rc = MBEDTLS_ERR_NET_INVALID_CONTEXT;

	if (bind_ctx && (netctx_getfd(accepting_ctx) != -1))
		rc = mbedtls_net_accept(bind_ctx, accepting_ctx, 0, 0, 0);

	return rc;
}


int netctx_set_blocking(mbedtls_net_context* ctx, int blocking)
{
	int rc = MBEDTLS_ERR_NET_INVALID_CONTEXT;

	if (netctx_getfd(ctx) != -1) {
		rc = blocking
			? mbedtls_net_set_block(ctx)
			: mbedtls_net_set_nonblock(ctx);

		if (rc)
			rc = MBEDTLS_ERR_NET_INVALID_CONTEXT;
	}

	return rc;
}


int netctx_set_nodelay(mbedtls_net_context* ctx, int nodelay)
{
	int rc = MBEDTLS_ERR_NET_INVALID_CONTEXT;

	if (netctx_getfd(ctx) != -1) {
		const int tcp_nodelay = nodelay ? 1 : 0;
		rc = setsockopt(
				ctx->fd, 
				IPPROTO_TCP, 
				TCP_NODELAY, 
				(const char *)&tcp_nodelay, 
				sizeof(tcp_nodelay));

		if (rc)
			rc = MBEDTLS_ERR_NET_INVALID_CONTEXT;
	}

	return rc;
}


int netctx_get_port(mbedtls_net_context* ctx)
{
	int port = -1;

	if (ctx) {
		struct sockaddr_storage sock_addr;
		int len = sizeof(sock_addr);

		if (getsockname(ctx->fd, (struct sockaddr *)&sock_addr, &len) == 0) {
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


netctx_poll_status netctx_poll(mbedtls_net_context* ctx, netctx_poll_mode mode, uint32_t timeout)
{
	netctx_poll_status status = { NETCTX_POLL_ERROR, MBEDTLS_ERR_NET_INVALID_CONTEXT, 0 };
	uint32_t rw = 0;

	// build the read/write flag
	if (mode.read == 1)
		rw = MBEDTLS_NET_POLL_READ;
	if (mode.write == 1)
		rw |= MBEDTLS_NET_POLL_WRITE;

	if (ctx && rw) {
		// wait to be ready for read or write
		const int rc = mbedtls_net_poll(ctx, rw, timeout);
		if (rc < 0) {
			// an error has occurred
			status.errnum = rc;
		}
		else if (rc == 0) {
			// wait timed out
			status.code = NETCTX_POLL_ERROR;
			status.errnum = MBEDTLS_ERR_SSL_TIMEOUT;
		}
		else {
			// ready for read and/or write
			status.code = NETCTX_POLL_OK;
			status.errnum = 0;
			status.event.read = rc & MBEDTLS_NET_POLL_READ;
			status.event.write = rc & MBEDTLS_NET_POLL_WRITE;
		}
	}

	return status;
}


netctx_snd_status netctx_send(mbedtls_net_context* ctx, const unsigned char* buf, size_t len)
{
	netctx_snd_status status = { NETCTX_SND_ERROR, MBEDTLS_ERR_NET_INVALID_CONTEXT, 0 };

	if (ctx) {
		int rc = mbedtls_net_send(ctx, buf, len);

		if (rc > 0) {
			status.code = NETCTX_SND_OK;
			status.errnum = 0;
			status.sbytes = rc;
		}
		else if (rc == MBEDTLS_ERR_SSL_WANT_WRITE) {
			status.code = NETCTX_SND_RETRY;
			status.errnum = rc;
		}
		else {
			status.code = NETCTX_SND_ERROR;
			status.errnum = rc;
		}
	}

	return status;
}


netctx_rcv_status netctx_recv(mbedtls_net_context* ctx, unsigned char* buf, size_t len)
{
	netctx_rcv_status status = { NETCTX_RCV_ERROR, MBEDTLS_ERR_NET_INVALID_CONTEXT, 0 };

	if (ctx) {
		int rc = mbedtls_net_recv(ctx, buf, len);

		if (rc > 0) {
			status.code = NETCTX_RCV_OK;
			status.errnum = 0;
			status.rbytes = rc;
		}
		else if (rc == 0) {
			status.code = NETCTX_RCV_EOF;
			status.errnum = 0;
		}
		else if (rc == MBEDTLS_ERR_SSL_WANT_READ) {
			status.code = NETCTX_RCV_RETRY;
			status.errnum = rc;
		}
		else {
			status.code = NETCTX_RCV_ERROR;
			status.errnum = rc;
		}
	}

	return status;
}
