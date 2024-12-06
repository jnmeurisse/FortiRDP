/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Cookie.h"

#include <vector>
#include "http/CookieError.h"
#include "tools/StrUtil.h"


namespace http {

	Cookie::Cookie(const tools::obfstring& value)
	{
		if (!parse(value)) {
			throw CookieError{ "Invalid cookie value:" + value.uncrypt() };
		}
	}


	tools::obfstring Cookie::to_header() const
	{
		return tools::obfstring{ _name + "=" }.append(_value);
	}


	bool Cookie::parse(const tools::obfstring& value)
	{
		bool rc = false;
		std::vector<tools::obfstring> parts;

		// Split the cookie string. There should always have one cookie-pair pair.
		// This pair contains the name and the value according
		// to the syntax : 
		//    cookie-pair *( ";" SP cookie-av )
		//    cookie-pair       = cookie-name "=" cookie-value
		if (tools::split(value, ';', parts) > 0) {
			const tools::obfstring cookie_pair(parts[0]);

			// split cookie-pair 
			const std::string::size_type pos = cookie_pair.find('=');
			if (pos != std::string::npos) {
				_name = cookie_pair.substr(0, pos).uncrypt();
				_value = tools::obfstring{ cookie_pair.substr(pos + 1, std::string::npos) };

				rc = true;
			}
			else {
				// invalid cookie
				rc = false;
			}

			if (rc && parts.size() > 1) {
				for (size_t idx = 1; idx < parts.size(); idx++) {
					// get next attribute-value
					const std::string cookie_av{ tools::trimleft(parts[idx].uncrypt()) };
					if (cookie_av.size() > 0) {
						// extract attribute name and value
						const std::string::size_type pos = cookie_av.find('=');

						if (pos != std::string::npos) {
							const std::string attribute_name{ cookie_av.substr(0, pos) };
							const std::string attribute_value{ cookie_av.substr(pos + 1, std::string::npos) };

							if (tools::iequal(attribute_name, "domain")) {
								_domain = attribute_value;
							}
							else if (tools::iequal(attribute_name, "path")) {
								_path = attribute_value;
							}
							else if (tools::iequal(attribute_name, "expires")) {
								_expires = attribute_value;
							}
						}
						else if (tools::iequal(cookie_av, "secure")) {
							_secure = true;
						}
						else if (tools::iequal(cookie_av, "httponly")) {
							_http_only = true;
						}
					}
				}
			}
		}

		return rc;
	}

}
