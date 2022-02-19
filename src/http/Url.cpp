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

	Url::Url(const std::string& url_path)
	{
		size_t pos = url_path.find('?');

		if (pos != std::string::npos) {
			_path = url_path.substr(0, pos);
			_query.add(url_path.substr(pos + 1, std::string::npos), '&');

		}
		else {
			_path = url_path;
		}
	}

}