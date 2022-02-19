/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <Windows.h>
#include <string>

namespace tools {

	/**
	 * A Path object represents a location of a file or a folder on a disk.
	 * A folder must finish by a \
	*/
	class Path {
	public:
		explicit Path();
		explicit Path(const std::wstring& path);
		explicit Path(const std::wstring& folder, const std::wstring& filename);
		~Path();

		/* assigns this path
		*/
		Path& operator= (const Path& other);

		/* returns the folder name, including the last backslash
		*/
		inline const std::wstring& folder() const noexcept { return _folder; }

		/* returns the name of file
		*/
		inline const std::wstring& filename() const noexcept { return _fname; }
		
		/* returns the combined folder and file name
		*/
		std::wstring to_string() const;

		/* returns the path truncated to the specified number of characters
		*/
		std::wstring compact(int max_char) const;

		/* returns the path to the specified module
		*/
		static Path get_module_path(HMODULE hModule = NULL);

		/* returns the path to the desktop
		*/
		static Path get_desktop_path();

	private:
		// The folder name (including the last \ delimiter)
		std::wstring _folder;

		// The filename
		std::wstring _fname;
	};

}