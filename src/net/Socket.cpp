/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/

#include "net/Socket.h"
#include "tools/StrUtil.h"


namespace net {
	static long bio_callback_fn(::BIO *bio, int oper, const char *argp, size_t len,
		int argi, long argl, int ret, size_t *processed)
	{
		Logger* const _logger = (Logger *)BIO_get_callback_arg(bio);

		if (_logger && _logger->is_trace_enabled()) {
			switch (oper) {
			case BIO_CB_FREE:
				_logger->trace(".... enter Free BIO=%x", bio);
				break;

			case BIO_CB_READ:
				_logger->trace(".... enter Read BIO=%x len=%d", bio, len);
				break;

			case BIO_CB_WRITE:
				_logger->trace(".... enter Write BIO=%x len=%d", bio, len);
				break;

			case BIO_CB_CTRL:
				_logger->trace(".... enter Ctrl BIO=%x arg=%d", bio, argi);
				break;

			case BIO_CB_GETS:
				_logger->trace(".... enter Gets BIO=%x arg=%d", bio, argi);
				break;

			case BIO_CB_PUTS:
				_logger->trace(".... enter Puts BIO=%x arg=%d", bio, argi);
				break;

			case BIO_CB_READ | BIO_CB_RETURN:
				_logger->trace(".... leave Read BIO=%x len=%d rc=%d", bio, len, ret);
				break;

			case BIO_CB_WRITE | BIO_CB_RETURN:
				_logger->trace(".... leave Write BIO=%x len=%d rc=%d", bio, len, ret);
				break;

			case BIO_CB_CTRL | BIO_CB_RETURN:
				_logger->trace(".... leave Ctrl BIO=%x arg=%d rc=%d", bio, argi, ret);
				break;

			case BIO_CB_GETS | BIO_CB_RETURN:
				_logger->trace(".... leave Gets BIO=%x arg=%d rc=%d", bio, argi, ret);
				break;

			case BIO_CB_PUTS | BIO_CB_RETURN:
				_logger->trace(".... leave Puts BIO=%x arg=%d rc=%d", bio, argi, ret);
				break;

			default:
				_logger->trace(".... undefined callback BIO=%x", bio);
				break;
			}
		}

		return ret;
	};


	Socket::Socket(::BIO* bio) :
		_logger(Logger::get_logger()),
		_bio(bio),
		_connected(false)
	{
		DEBUG_CTOR(_logger, "Socket");

		if (!_bio)
			throw std::invalid_argument("null bio in Socket ctor");

		if (_logger->is_trace_enabled()) {
			_logger->trace("... %x allocate BIO=%x", this, _bio);
			::BIO_set_callback_ex(_bio, bio_callback_fn);
			::BIO_set_callback_arg(_bio, (char *) _logger);
		}
	}


	Socket::~Socket()
	{
		DEBUG_DTOR(_logger, "Socket");

		if (_logger->is_trace_enabled()) {
			_logger->trace("... %x free BIO %x", this, _bio);
		}
		
		::BIO_free_all(_bio);
	}


	bool Socket::connect(const Endpoint& ep, int timeout)
	{
		DEBUG_ENTER(_logger, "Socket", "connect");

		if (_connected)
			throw std::runtime_error("socket is open");

		_connected = set_endpoint(ep) && do_connect(timeout);
		return _connected;
	}

	
	bool Socket::close()
	{
		DEBUG_ENTER(_logger, "Socket", "close");

		_connected = false;
		return BIO_reset(_bio) == 1;
	}


	Endpoint Socket::get_endpoint() const
	{
		const std::string hostname{ BIO_get_conn_hostname(_bio) };
		const std::string portnum{ BIO_get_conn_port(_bio) };
		int port = 0;

		return tools::str2i(portnum, port) ? Endpoint(hostname, port) : Endpoint();
	}


	Socket::rcv_status Socket::recv(byte* buf, const size_t len, size_t& rbytes)
	{
		DEBUG_ENTER(_logger, "Socket", "recv");

		rcv_status status;
		int rc = ::BIO_read(_bio, buf, (int)len);

		if (rc > 0) {
			rbytes = rc;
			status = rcv_status::rcv_ok;
		}
		else if (rc == 0) {
			rbytes = 0;
			status = rcv_status::rcv_eof;
		}
		else {
			if (BIO_should_retry(_bio))
				status = rcv_status::rcv_retry;
			else
				status = rcv_status::rcv_error;
		}

		/* 
			This code is not working with OpenSSL 3.x (see issue #8208)
			BIO_read_ex does not distinguish EOF from failure.
		
		int rc = ::BIO_read_ex(_bio, buf, len, &rbytes);
		rcv_status status;

		if (rc == 1) {
			if (rbytes <= 0)
				status = rcv_status::rcv_eof;
			else
				status = rcv_status::rcv_ok;
		}
		else {
			if (BIO_should_retry(_bio))
				status = rcv_status::rcv_retry;
			else
				status = rcv_status::rcv_error;
		}
		*/

		return status;
	}


	Socket::snd_status Socket::send(const byte* buf, const size_t len, size_t& sbytes)
	{
		DEBUG_ENTER(_logger, "Socket", "send");

		int rc = ::BIO_write_ex(_bio, buf, len, &sbytes);
		snd_status status;

		if (rc == 1)
			status = snd_status::snd_ok;
		else if (BIO_should_retry(_bio))
			status = snd_status::snd_retry;
		else
			status = snd_status::snd_error;

		return status;
	}


	bool Socket::flush()
	{
		DEBUG_ENTER(_logger, "Socket", "flush");

		return (BIO_flush(_bio) == 1) || BIO_should_retry(_bio);
	}


	int Socket::get_fd() const
	{
		int fd;
		return BIO_get_fd(_bio, &fd);
	}


	bool Socket::is_connected() const noexcept
	{
		return _connected;
	}


	bool Socket::set_endpoint(const Endpoint& ep)
	{
		const std::string& conn_host{ ep.hostname() };
		const std::string conn_port{ std::to_string(ep.port()) };

		return (::BIO_set_conn_port(_bio, conn_port.c_str()) == 1) &&
			(::BIO_set_conn_hostname(_bio, conn_host.c_str()) == 1);
	}
}
