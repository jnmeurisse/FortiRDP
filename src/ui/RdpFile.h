/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <map>
#include <list>

#include "tools/Path.h"


struct RdpOption
{
	std::string option_name;
	std::string option_type;
	std::string option_value;
};


class RdpFile final
{
public:
	explicit RdpFile(const tools::Path& path);

	inline bool empty() const { return _options_list.size() == 0; }
	bool read();
	bool write();

	void set(const std::string& name, int value);
	void set(const std::string& name, const std::string& value);

	bool get(const std::string& name, std::string& type, std::string& value);

private:
	const tools::Path _path;
	std::list<RdpOption> _options_list;

	RdpOption* find(const std::string& name);
	void add_option(const std::string& name, const std::string& type, const std::string& value);
	void set(const std::string& name, const std::string& type, const std::string& value);
};
