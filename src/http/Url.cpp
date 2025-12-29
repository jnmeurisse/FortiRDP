/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Url.h"

#include <cctype>
#include "net/Endpoint.h"
#include "tools/StrUtil.h"


namespace http {

	Url::Url(const std::string& url) : 
		Url()
	{
		std::string buffer;
		buffer.reserve(url.length());

		// pointer to next character
		const char* p = url.c_str();

		// skip spaces
		while (*p && std::isspace(*p))
			p++;

		// This code is a URL Parser implemented as a Finite State Machine(FSM).
		// It iterates through a string character by character and breaks it down
		// into its components.
		//
		// State		URL Component	Description
        //	0, 1		Scheme,			Identifies the protocol (e.g., http, https).
        //	2, 3, 4		Transition		Handles the // that usually follows a scheme (e.g., http://).
        //	5			Authority		Captures the domain or host (e.g., www.example.com).
        //	7			Path			Captures the resource path (e.g., /index.html).
        //	8, 9		Query			Captures parameters after the ? (e.g., search=cpp)
        //	10, 11		Fragment		Captures the "anchor" or fragment after the #.

		int state = 0;
		while (state >= 0) {
			switch (state) {
			case 0:
				if (*p == '\0') {
					state = -1;
				}
				else if (*p == ':') {
					buffer = *p; state = 7;
				}
				else if (*p == '/') {
					buffer = *p; state = 3;
				}
				else if (*p == '?') {
					state = 8;
				}
				else if (*p == '#') {
					state = 10;
				}
				else {
					buffer = *p; state = 1;
				}
				break;

			case 1:
				if (*p == '\0') {
					_path = buffer; state = -1;
				}
				else if (*p == ':') {
					_scheme = buffer; buffer.clear(); state = 2;
				}
				else if (*p == '?') {
					_path = buffer; state = 8;
				}
				else if (*p == '#') {
					_path = buffer; state = 10;
				}
				else {
					buffer.push_back(*p); state = 1;
				}
				break;

			case 2:
				if (*p == '\0') {
					state = -1;
				}
				else if (*p == '/') {
					state = 3;
				}
				else {
					buffer = *p; state = 7;
				}
				break;

			case 3:
				if (*p == '\0') {
					_path = buffer; state = -1;
				}
				else if (*p == '/') {
					state = 4;
				}
				else {
					buffer.push_back(*p); state = 7;
				}
				break;

			case 4:
				if (*p == '\0') {
					state = -1;
				}
				else {
					buffer = *p; state = 5;
				}
				break;

			case 5:
				if (*p == '\0') {
					_authority = buffer; state = -1;
				}
				else if (*p == '/') {
					_authority = buffer; buffer = "/"; state = 7;
				}
				else if (*p == '?') {
					_authority = buffer; state = 8;
				}
				else if (*p == '#') {
					_authority = buffer; state = 10;
				}
				else {
					buffer.push_back(*p); state = 5;
				}
				break;

			case 6:
				if (*p == '\0') {
					state = -1;
				}
				else {
					buffer.push_back(*p); state = 7;
				}
				break;

			case 7:
				if (*p == '\0') {
					_path = buffer; state = -1;
				}
				else if (*p == '?') {
					_path = buffer; buffer.clear(); state = 8;
				}
				else if (*p == '#') {
					_path = buffer; buffer.clear(); state = 10;
				}
				else {
					buffer.push_back(*p); state = 7;
				}
				break;

			case 8:
				if (*p == '\0') {
					state = -1;
				}
				else {
					buffer = *p; state = 9;
				}
				break;

			case 9:
				if (*p == '\0') {
					_query = buffer; state = -1;
				}
				else if (*p == '#') {
					_query = buffer; buffer.clear(); state = 10;
				}
				else {
					buffer.push_back(*p); state = 9;
				}
				break;

			case 10:
				if (*p == '\0') {
					state = -1;
				}
				else {
					buffer = *p; state = 11;
				}
				break;

			case 11:
				if (*p == '\0') {
					_fragment = buffer; state = -1;
				}
				else {
					buffer.push_back(*p); state = 11;
				}
				break;

			default:
				break;
			}

			// move to next character
			if (state >= 0)
				p++;
		}
	}


	Url::Url(const std::string& scheme, const std::string& authority,
		const std::string& path, const std::string& query, const std::string& fragment) :
		_scheme(tools::trim(scheme)),
		_authority(tools::trim(authority)),
		_path(tools::trim(path)),
		_query(tools::trim(query)),
		_fragment(tools::trim(fragment))
	{
	}


	std::string Url::get_hostname() const
	{
		std::string hostname;

		if (!_authority.empty()) {
			const net::Endpoint end_point{ _authority, 80 };
			hostname = end_point.hostname();
		}

		return hostname;
	}


	tools::StringMap Url::get_query_map() const
	{
		return tools::StringMap(_query, '&');
	}


	std::string Url::to_string(bool implicit) const
	{
		std::string url;

		if (implicit) {
			url.append(_path);
			if (_query.length() > 0)
				url.append("?").append(_query);

			if (_fragment.length() > 0) 
				url.append("#").append(_fragment);
		} 
		else {
			if (_scheme.length() > 0)
				url.append(_scheme).append(":");

			if (_authority.length() > 0)
				url.append("//").append(_authority);

			url.append(_path);

			if (_query.length() > 0)
				url.append("?").append(_query);

			if (_fragment.length() > 0)
				url.append("#").append(_fragment);
		}

		return url;
	}

}
