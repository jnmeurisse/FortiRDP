/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <vector>
#include "http/Url.h"

namespace http {

	Url::Url(const std::string& url)
	{
		size_t pos = url.find('?');

		if (pos != std::string::npos) {
			_path = url.substr(0, pos);
			_query.add(url.substr(pos + 1, std::string::npos), '&');

		}
		else {
			_path = url;
		}
	}

}