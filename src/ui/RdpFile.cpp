/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "RdpFile.h"

#include <iostream>
#include <fstream>
#include <vector>
#include "util/StrUtil.h"

namespace ui {
	RdpFile::RdpFile(const utl::Path& path) :
		_path(path)
	{
	}


	bool RdpFile::read()
	{
		bool rc = false;
		std::ifstream input_file(_path.to_string());

		if (input_file.is_open()) {
			std::string line;
			std::vector<std::string> params;

			while (std::getline(input_file, line))
			{
				// Skip invalid lines
				if (line.size() > 1024)
					break;

				// Split option parameters
				if (utl::str::split(line, ':', params) == 3)
				{
					// Add it to the list
					add_option(params[0], params[1], params[2]);
				}
			}

			input_file.close();

			rc = true;
		}

		return rc;
	}


	bool RdpFile::write()
	{
		bool rc = false;
		std::ofstream ofs{ _path.to_string(), std::ostream::out };

		if (ofs.is_open() && !empty()) {
			for (auto const& opt : _options_list) {
				ofs << opt.option_name << ":" << opt.option_type << ":" << opt.option_value << std::endl;
			}

			ofs.close();
			rc = true;
		}

		return rc;
	}


	void RdpFile::set(const std::string& name, int value)
	{
		set(name, "i", std::to_string(value));
	}


	void RdpFile::set(const std::string& name, const std::string& value)
	{
		set(name, "s", value);
	}


	bool RdpFile::get(const std::string& name, std::string& type, std::string& value)
	{
		const RdpOption* const option = find(name);

		if (option) {
			value = option->option_value;
			type = option->option_type;
		}

		return option != nullptr;
	}


	RdpOption* RdpFile::find(const std::string& name)
	{
		for (auto& it : _options_list) {
			if (utl::str::iequal(name, it.option_name))
				return &it;
		}

		return nullptr;
	}


	void RdpFile::add_option(const std::string& name, const std::string& type, const std::string& value)
	{
		RdpOption option;
		option.option_name = name;
		option.option_type = type;
		option.option_value = value;

		// Add it to the list of options
		_options_list.push_back(option);
	}


	void RdpFile::set(const std::string& name, const std::string& type, const std::string& value)
	{
		RdpOption* const option = find(name);

		if (option) {
			// If yes, update the value
			option->option_type = type;
			option->option_value = value;
		}
		else {
			// Add the option at the end of the list
			add_option(name, type, value);
		}
	}

}
