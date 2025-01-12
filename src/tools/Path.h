/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <Windows.h>
#include <ShlObj.h>
#include <string>



namespace tools {

	/**
	 * A Path object represents a location of a file or a folder on a disk.
	 * A folder must finish by a \
	*/
	class Path final 
	{
	public:
		Path() = default;
		explicit Path(const std::wstring& path);
		explicit Path(const std::wstring& folder, const std::wstring& filename);

		/* returns the folder name, including the last backslash
		*/
		inline const std::wstring& folder() const noexcept { return _folder; }

		/* returns the file name
		*/
		inline const std::wstring& filename() const noexcept { return _fname; }
		
		/* returns the combined folder and file name
		*/
		std::wstring to_string() const;

		/* returns the path truncated to the specified number of characters
		*/
		std::wstring compact(unsigned int max_char) const;

		/* returns the path to the specified module
		*/
		static Path get_module_path(HMODULE hModule = NULL);

		/* returns the path to the desktop
		*/
		static Path get_desktop_path();

		/* returns the path to AppData/local
		*/
		static Path get_appdata_path();

	private:
		// The folder name (including the last \ delimiter)
		std::wstring _folder;

		// The filename
		std::wstring _fname;

		// returns the path to a windows known folder.
		static Path get_known_folder_path(REFKNOWNFOLDERID rfid);
	};

}
