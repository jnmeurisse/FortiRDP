/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <openssl/err.h>
#include "net/Listener.h"
#include "net/TcpSocket.h"


namespace net {

	Listener::Listener() :
		_logger(Logger::get_logger()),
		_bio(::BIO_new(::BIO_s_accept()))
	{
		DEBUG_CTOR(_logger, "Listener");

		if (!_bio)
			throw std::bad_alloc();
	}


	Listener::~Listener()
	{
		DEBUG_DTOR(_logger, "Listener");
		close();
		BIO_vfree(_bio);
	}


	bool Listener::bind(const Endpoint& endpoint)
	{
		DEBUG_ENTER(_logger, "Listener", "bind");

		if (BIO_set_accept_name(_bio, endpoint.to_string().c_str()) != 1) {
			_logger->trace("ERROR: Listener::bind set_accept_name error %d", ERR_peek_error());
			return false;
		}

		// set the socket in non blocking mode
		if (BIO_set_nbio_accept(_bio, 1) != 1) {
			_logger->trace("ERROR: Listener::bind set_nbio_accept error %d", ERR_peek_error());
			return false;
		}

		return ::BIO_do_accept(_bio) == 1;
	}


	Socket* Listener::accept()
	{
		DEBUG_ENTER(_logger, "Listener", "accept");

		if (::BIO_do_accept(_bio) != 1)
			return nullptr;

		return new TcpSocket(BIO_pop(_bio));
	}


	bool Listener::close()
	{
		DEBUG_ENTER(_logger, "Listener", "close");
		
		return BIO_reset(_bio) == 1;
	}


	Endpoint Listener::endpoint() const
	{
		const char* const name = BIO_get_accept_name(_bio);
		return name? Endpoint(name, 0) : Endpoint();
	}


	bool Listener::is_ready() const
	{
		int opt_val;
		int opt_len = sizeof(opt_val);

		if (getsockopt(
				get_fd(),
				SOL_SOCKET,
				SO_ACCEPTCONN,
				(char*)&opt_val,
				&opt_len) == SOCKET_ERROR)
			return false;

		return opt_val != 0;
	}


	int Listener::get_fd() const
	{
		int fd;
		return BIO_get_fd(_bio, &fd);
	}

}