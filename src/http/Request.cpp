/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <openssl/err.h>
#include "tools/Timer.h"
#include "Request.h"

namespace http {


	/* Initialization of common HTTP Verbs */
	const std::string Request::GET_VERB = "GET";
	const std::string Request::POST_VERB = "POST";
	const std::string Request::PUT_VERB = "PUT";
	const std::string Request::DELETE_VERB = "DELETE";
	const std::string Request::HEAD_VERB = "HEAD";
	const std::string Request::OPTIONS_VERB = "OPTIONS";
	const std::string Request::TRACE_VERB = "TRACE";


	Request::Request(const std::string& verb, const Url& url, const Cookies& cookies, int timeout) :
		_logger(Logger::get_logger()),
		_verb(verb),
		_url(url),
		_cookies(cookies),
		_headers(),
		_body(2048),
		_timeout(timeout)
	{
	}


	void Request::clear()
	{
		_headers.serase();
		_body.clear();
	}


	Request& Request::set_body(const byte* data, size_t size)
	{
		_body.clear();
		_body.append(data, size);

		return *this;
	}


	void Request::send(Socket& socket)
	{
		DEBUG_ENTER(_logger, "Request", "send");

		ByteBuffer buffer(1024);
		bool rc;

		if (_body.size() > 0) {
			// add a content-length header if there is a body.
			_headers.set("Content-Length", _body.size());
		}

		buffer
			.append(_verb).append(" ")
			.append(_url.to_string(true)).append(" ")
			.append("HTTP/1.1")
			.append("\r\n");

		// Append all headers
		_headers.write(buffer);

		// add cookies, cookies are still obfuscated at this stage
		obfstring cookie_header{ _cookies.to_header() };
		if (cookie_header.size() > 0) {
			// cookies are appended decrypted in the buffer
			buffer
				.append("Cookie: ")
				.append(cookie_header)
				.append("\r\n");
		}
		buffer.append("\r\n");

		// Send headers to the web server
		rc = write_buffer(socket, buffer.cbegin(), buffer.size());
		if (_logger->is_trace_enabled())
			_logger->trace("... %x       Request::send : write headers rc=%d", this, rc);

		if (!rc)
			throw ossl_error(ERR_peek_error());

		// Erase sensitive data
		buffer.clear();

		if (_body.size() > 0) {
			// send the body to the web server
			rc = write_buffer(socket, _body.cbegin(), _body.size());
			if (_logger->is_trace_enabled())
				_logger->trace("... %x       Request::send : write body rc=%d", this, rc);

			if (!rc)
				throw ossl_error(ERR_peek_error());
		}

		// flush the output buffer
		rc = socket.flush();
		if (_logger->is_trace_enabled())
			_logger->trace("... %x       Request::send : flush rc=%d", this, rc);

		if (!rc)
			throw ossl_error(ERR_peek_error());

		return;
	}


	bool Request::write(Socket& socket, const byte* buffer, size_t len, size_t &sbytes)
	{
		tools::Timer timer{ _timeout * 1000 };
		bool abort = false;

		do {
			switch (socket.send(buffer, len, sbytes)) {
			case Socket::snd_retry:
				if (timer.elapsed()) {
					ERR_raise(ERR_LIB_BIO, BIO_R_TRANSFER_TIMEOUT);
					abort = true;
				}
				else
					Sleep(150);
				break;

			case Socket::snd_ok:
				return true;

			case Socket::snd_error:
				abort = true;
				break;
			}
		} while (!abort);

		throw ossl_error(ERR_peek_error());
	}


	bool Request::write_buffer(Socket& socket, const byte* buffer, size_t len)
	{
		while (len > 0) {
			size_t sbytes = 0;
			if (!write(socket, buffer, len, sbytes))
				return  false;

			len -= sbytes;
			buffer += sbytes;
		}

		return true;
	}

}
