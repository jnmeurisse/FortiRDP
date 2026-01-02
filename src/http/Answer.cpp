/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Answer.h"

#include <algorithm>
#include <string>
#include <zlib.h>
#include "http/Cookie.h"
#include "http/CookieError.h"
#include "http/HttpError.h"
#include "tools/StrUtil.h"
#include "tools/ErrUtil.h"
#include <cstring>
#include <cctype>
#include <locale>
#include <array>


namespace http {
	using namespace net;

	const int default_code = 400;
	const std::string default_reason = "Bad Request";

	Answer::Answer() :
		_logger(Logger::get_logger()),
		_status_code(default_code),
		_reason_phrase(default_reason),
		_headers(),
		_cookies(),
		_body(4096)
	{
		DEBUG_CTOR(_logger);
	}


	Answer::~Answer()
	{
		DEBUG_DTOR(_logger);
	}


	void Answer::clear()
	{
		DEBUG_ENTER(_logger);

		_status_code = default_code;
		_reason_phrase = default_reason;
		_body.clear();
		_headers.serase();
		_cookies.clear();
	}


	bool Answer::read_buffer(net::TcpSocket& socket, unsigned char* buffer, const size_t len, const tools::Timer& timer)
	{
		TRACE_ENTER_FMT(_logger, "buffer=0x%012Ix size=%zu timeout=%lu",
			PTR_VAL(buffer),
			len,
			timer.remaining_time()
		);

		const rcv_status status{ socket.read(buffer, len, timer) };
		switch (status.code) {
			case rcv_status_code::NETCTX_RCV_ERROR:
			case rcv_status_code::NETCTX_RCV_RETRY:
				// read failed or timed out
				throw mbed_error(status.rc);

			case rcv_status_code::NETCTX_RCV_OK:
				return true;

			case rcv_status_code::NETCTX_RCV_EOF:
			default:
				return false;
		}
	}


	bool Answer::read_byte(net::TcpSocket& socket, unsigned char& c, const tools::Timer& timer)
	{
		return read_buffer(socket, &c, sizeof(char), timer);
	}


	Answer::answer_status Answer::read_line(net::TcpSocket& socket, tools::ByteBuffer& buffer, const tools::Timer& timer)
	{
		TRACE_ENTER_FMT(_logger, "buffer=0x%012Ix size=%zu timeout=%lu",
			PTR_VAL(std::addressof(buffer)),
			buffer.size(),
			timer.remaining_time()
		);

		int state = 0;
		size_t bytes_received = 0;

		buffer.clear();

		while (state != 2) {
			unsigned char c;

			if (!read_byte(socket, c, timer))
				return answer_status::ERR_EOF;

			switch (state) {
			case 0:
				if (c == '\r')
					state = 1;
				else if (bytes_received < buffer.capactity())
					buffer.append(&c, 1);
				break;

			case 1:
				if (c == '\n')
					state = 2;
				else if (bytes_received < buffer.capactity()) {
					buffer.append('\r');
					buffer.append(c);
				}
				break;

			default:
				break;
			}

			bytes_received++;
		}

		return answer_status::ERR_NONE;
	}


	Answer::answer_status Answer::read_control_data(net::TcpSocket& socket, const tools::Timer& timer)
	{
		DEBUG_ENTER(_logger);

		// We assume that the server did not understand our send_request
		_status_code = default_code;
		_reason_phrase = default_reason;

		// The answer should have the following syntax :
		//		HTTP-Version SP Status-Code SP Reason-Phrase
		// See https://www.rfc-editor.org/rfc/rfc9110.html

		// Read the HTTP version. Return an error code if the server has closed 
		// the connection before sending a valid response.
		std::array<unsigned char, 8> http_version = {};
		if (!read_buffer(socket, http_version.data(), http_version.size(), timer))
			return answer_status::ERR_INVALID_STATUS_LINE;
		if (std::memcmp(http_version.data(), "HTTP/1.1", http_version.size()) != 0)
			return answer_status::ERR_INVALID_VERSION;

		// Skip a space
		unsigned char space;
		if (!read_buffer(socket, &space, sizeof(space), timer) || space != ' ')
			return answer_status::ERR_INVALID_STATUS_LINE;

		// Read the status code.
		// The status code of a response is a three-digit integer code that describes
		// the result of the send_request.
		std::array<unsigned char, 3> status_code = {};
		if (!read_buffer(socket, status_code.data(), status_code.size(), timer))
			return answer_status::ERR_INVALID_STATUS_LINE;

		// Convert status code text to an integer.
		_status_code = 0;
		for (unsigned char c : status_code) {
			if (!std::isdigit(c))
				return answer_status::ERR_INVALID_STATUS_CODE;
			_status_code = (_status_code * 10) + (c - '0');
		}

		if ((_status_code < 100) || (_status_code >= 600))
			// Invalid HTTP code
			return answer_status::ERR_INVALID_STATUS_CODE;

		// Read the reason phrase
		tools::ByteBuffer buffer(1024);
		answer_status status = read_line(socket, buffer, timer);
		if (status == answer_status::ERR_NONE  && !buffer.empty()) {
			_reason_phrase = tools::trim(buffer.to_string());
		}

		return status;
	}


	static bool is_valid_field_name(const std::string& field_name)
	{
		auto& facet = std::use_facet<std::ctype<char>>(std::locale::classic());
		
		/* Field names should be restricted to just letters, digits, and 
		 * hyphen('-') characters. */
		return
			field_name.size() > 0 &&
			std::all_of(
				field_name.begin(),
				field_name.end(),
				[&facet](unsigned char c) {
					return facet.is(std::ctype_base::alnum, c) || c == '-'; 
				}
			);
	}


	Answer::answer_status Answer::read_headers(net::TcpSocket& socket, const tools::Timer& timer)
	{
		DEBUG_ENTER(_logger);

;		answer_status status;
		tools::ByteBuffer buffer(MAX_HEADER_SIZE);

		while ((status = read_line(socket, buffer, timer)) == answer_status::ERR_NONE && !buffer.empty()) {
			tools::obfstring line{ buffer.to_obfstring() };

			// Split header into name and value at the first colon.
			const std::string::size_type pos{ line.find(':') };
			if (pos != std::string::npos && pos > 0) {
				const std::string field_name{ line.substr(0, pos).uncrypt() };

				// Validate the field name.
				if (!is_valid_field_name(field_name))
					return answer_status::ERR_INVALID_FIELD;

				const tools::obfstring field_value{ tools::trim(line.substr(pos + 1, std::string::npos)) };

				if (tools::iequal(field_name, "Set-Cookie")) {
					// A cookie definition
					try {
						_cookies.add(Cookie::parse(field_value));
					}
					catch (const CookieError& e) {
						_logger->debug("ERROR: %s", e.what());
					}
				}
				else {
					// It is a header.
					_headers.set(field_name, field_value.uncrypt());
				}
			}
		}

		return status;
	}


	bool Answer::read_gzip_body(net::TcpSocket& socket, size_t size, size_t max_size, const tools::Timer& timer)
	{
		DEBUG_ENTER(_logger);

		// configure the de-compressor
		::z_stream strm{ 0 };
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = 0;
		strm.next_in = Z_NULL;
		if (inflateInit2(&strm, MAX_WBITS + 32))
			return false;

		// read by chunk of 1024 bytes
		unsigned char buffer[1024];
		unsigned char out[1024];

		do {
			const size_t len = std::min(size, sizeof(buffer));
			if (!read_buffer(socket, buffer, std::min(size, sizeof(buffer)), timer)) {
				inflateEnd(&strm);
				return false;
			}

			strm.avail_in = static_cast<uInt>(len);
			strm.next_in = buffer;

			do {
				strm.avail_out = sizeof(out);
				strm.next_out = out;

				switch (inflate(&strm, Z_NO_FLUSH)) {
					case Z_NEED_DICT:
					case Z_DATA_ERROR:
					case Z_MEM_ERROR:
						(void)inflateEnd(&strm);
						return false;
				}
				const size_t have = sizeof(out) - strm.avail_out;

				const size_t available_space = max_size - _body.size();
				if (available_space > 0) {
					_body.append(out, std::min(have, available_space));
				}
			} while (strm.avail_out == 0);

			size = size - len;
		} while (size > 0);

		return (inflateEnd(&strm) == Z_OK) && size == 0;
	}


	bool Answer::read_body(net::TcpSocket& socket, size_t size, size_t max_size, const tools::Timer& timer)
	{
		DEBUG_ENTER(_logger);

		// read by chunk of 4096 bytes
		std::array<unsigned char, 4096> buffer;

		do {
			// read a chunk of bytes
			const size_t len = std::min(size, buffer.size());
			if (!read_buffer(socket, buffer.data(), len, timer))
				break;

			// append what we can in the buffer
			const size_t available_space = max_size - _body.size();
			if (available_space > 0) {
				_body.append(buffer.data(), std::min(len, available_space));
			}

			size = size - len;
		} while (size > 0);

		// return True when all bytes have been read
		return size == 0;
	}


	void Answer::recv(net::TcpSocket& socket, const tools::Timer& timer)
	{
		DEBUG_ENTER(_logger);
		LOG_DEBUG(_logger, "timeout=%lu", timer.remaining_time());

		answer_status status;

		// The received message has the following syntax
		//  answer  = status-line CRLF
		//            *(message-header CRLF)
		//            CRLF
		//            [ message-body ]
		//  message-header = field-name ":" [ field-value ]
		//
		//  Note : leading or trailing LWS MAY be removed without changing 
		//         the semantics of the field value

		// Read status-line.
		if ((status = read_control_data(socket, timer)) != answer_status::ERR_NONE)
			throw http_error(answer_status_msg(status));

		// Read message-headers. 
		if ((status = read_headers(socket, timer)) != answer_status::ERR_NONE)
			throw http_error(answer_status_msg(status));

		// Get the encoding scheme
		std::string transfer_encoding;
		if (_headers.get("Transfer-Encoding", transfer_encoding)) {
			transfer_encoding = tools::lower(tools::trim(transfer_encoding));
		}

		bool gzip_content = false;
		std::string content_encoding;
		if (_headers.get("Content-Encoding", content_encoding)) {
			content_encoding = tools::lower(tools::trim(content_encoding));
			if (content_encoding.compare("") == 0) {
				// encoding not specified.
			}
			else if (content_encoding.compare("gzip") == 0) {
				gzip_content = true;
			}
			else {
				throw http_error(answer_status_msg(answer_status::ERR_CONTENT_ENCODING));
			}
		}

		// Read the body.
		// Are we using a chunked-style transfer encoding?
		if (transfer_encoding.compare("chunked") == 0) {
			if (gzip_content)
				throw http_error(answer_status_msg(answer_status::ERR_CONTENT_ENCODING));

			// chunked message
			long chunk_size = 0;
			tools::ByteBuffer buffer(MAX_LINE_SIZE);

			do {
				// read chunk size
				if (read_line(socket, buffer, timer) != answer_status::ERR_NONE || buffer.empty())
					throw http_error(answer_status_msg(answer_status::ERR_CHUNK_SIZE));

				// decode chunk size
				if (!tools::str2num(buffer.to_string(), 16, 0, MAX_CHUNK_SIZE, chunk_size)) {
					throw http_error(answer_status_msg(answer_status::ERR_CHUNK_SIZE));
				}

				if (chunk_size > 0) {
					if (!read_body(socket, chunk_size, MAX_BODY_SIZE, timer))
						throw http_error(answer_status_msg(answer_status::ERR_BODY));
				}

				// skip eol
				if (read_line(socket, buffer, timer) != answer_status::ERR_NONE || !buffer.empty())
					throw http_error(answer_status_msg(answer_status::ERR_BODY));
			} while (chunk_size > 0);

		}
		else if (transfer_encoding.compare("") == 0) {
			// read content length 
			std::string length;

			if (_headers.get("Content-Length", length)) {
				long size = 0;

				if (!tools::str2num(length, 10, 0, MAX_BODY_SIZE, size)) {
					throw http_error(answer_status_msg(answer_status::ERR_BODY_SIZE));
				}

				if (size > 0) {
					// define the capacity of the buffer
					_body.reserve(size);

					// read the whole body
					bool body_available;
					if (gzip_content)
						body_available = read_gzip_body(socket, size, MAX_BODY_SIZE, timer);
					else
						body_available = read_body(socket, size, MAX_BODY_SIZE, timer);

					if (!body_available)
						throw http_error(answer_status_msg(answer_status::ERR_BODY));
				}
			}
		}
		else {
			// unsupported transfer encoding
			throw http_error(answer_status_msg(answer_status::ERR_TRANSFER_ENCODING));
		}

		return;
	}


	std::string Answer::answer_status_msg(answer_status status)
	{
		std::string message;
		switch (status) {
		case answer_status::ERR_EOF:
			message = "HTTP answer EOF"; break;

		case answer_status::ERR_INVALID_FIELD:
			message = "HTTP answer contains non ASCII character"; break;

		case answer_status::ERR_INVALID_STATUS_LINE:;
			message = "Invalid HTTP status line"; break;

		case answer_status::ERR_INVALID_VERSION:
			message = "Invalid HTTP version"; break;

		case answer_status::ERR_INVALID_STATUS_CODE:
			message = "Invalid HTTP status code"; break;

		case answer_status::ERR_INVALID_HEADER:
			message = "Invalid HTTP header"; break;

		case answer_status::ERR_CHUNK_SIZE:
			message = "Invalid HTTP chunk size"; break;

		case answer_status::ERR_BODY_SIZE:
			message = "Invalid HTTP body size"; break;

		case answer_status::ERR_CONTENT_ENCODING:
			message = "Unsupported HTTP content encoding"; break;

		case answer_status::ERR_TRANSFER_ENCODING:
			message = "Unsupported HTTP transfer encoding"; break;

		case answer_status::ERR_BODY:
			message = "Incomplete HTTP body"; break;

		default:
			message = ""; break;
		}

		return message;
	}


	const char* Answer::__class__ = "Answer";

}
