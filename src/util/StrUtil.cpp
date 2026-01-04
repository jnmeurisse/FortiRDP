/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "StrUtil.h"

#include <Windows.h>
#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <vector>


namespace utl {

	namespace str {

		size_t split(const char* str, const char delim, std::vector<std::string>& parts)
		{
			const size_t count = parts.size();

			do {
				const char* const begin = str;

				while (*str != delim && *str)
					str++;

				parts.emplace_back(begin, str);
			} while (0 != *str++);

			return parts.size() - count;
		}


		size_t split(const std::string& str, const char delim, std::vector<std::string>& parts)
		{
			return split(str.c_str(), delim, parts);
		}


		size_t split(const wchar_t* str, const wchar_t delim, std::vector<std::wstring>& parts)
		{
			const size_t count = parts.size();

			do {
				const wchar_t* const begin = str;

				while (*str != delim && *str)
					str++;

				parts.emplace_back(begin, str);
			} while (0 != *str++);

			return parts.size() - count;
		}


		size_t split(const std::wstring& str, const wchar_t delim, std::vector<std::wstring>& parts)
		{
			return split(str.c_str(), delim, parts);
		}


		size_t split(const obfstring& str, const char delim, std::vector<obfstring>& parts)
		{
			const size_t count = parts.size();

			size_t index = 0;
			do {
				const size_t begin = index;
				while (index < str.size() && str[index] != delim)
					index++;

				parts.push_back(str.substr(begin, index - begin));
				index++;
			} while (index < str.size());

			return parts.size() - count;
		}


		bool str2num(const std::string& numstr, const int radix, const long minval, const long maxval, long& value)
		{
			if (minval > maxval) {
				errno = EINVAL;

			}
			else {
				errno = 0;
				long l = std::strtol(numstr.c_str(), nullptr, radix);

				if (errno == 0) {
					if (l < minval || l > maxval) {
						errno = ERANGE;
					}
					else {
						value = l;
					}
				}
			}

			return errno == 0;
		}


		bool str2i(const std::wstring& str, int& value)
		{
			return str2i(wstr2str(str), value);
		}


		bool str2i(const std::string& str, int& value)
		{
			long tmp;
			const bool ok = str2num(str, 10, INT_MIN, INT_MAX, tmp);
			if (ok)
				value = tmp;

			return ok;
		}


		static bool icheq(unsigned char a, unsigned char b)
		{
			return std::tolower(a) == std::tolower(b);
		}


		bool iequal(std::string const& s1, std::string const& s2)
		{
			return (s1.length() == s2.length()) && std::equal(s2.begin(), s2.end(), s1.begin(), icheq);
		}


		std::wstring trimright(const std::wstring& str)
		{
			return str.length() == 0 ? str : str.substr(0, str.find_last_not_of(L" \t") + 1);
		}


		std::wstring trimleft(const std::wstring& str)
		{
			return str.length() == 0 ? str : str.substr(str.find_first_not_of(L" \t"));
		}


		std::wstring trim(const std::wstring& str)
		{
			return str.length() == 0 ? str : trimleft(trimright(str));
		}


		std::string trimright(const std::string& str)
		{
			return str.length() == 0 ? str : str.substr(0, str.find_last_not_of(" \t") + 1);
		}


		std::string trimleft(const std::string& str)
		{
			return str.length() == 0 ? str : str.substr(str.find_first_not_of(" \t"));
		}


		std::string trim(const std::string& str)
		{
			return str.length() == 0 ? str : trimleft(trimright(str));
		}


		obfstring trimright(const obfstring& str)
		{
			return str.length() == 0 ? str : str.substr(0, str.find_last_not_of(" \t") + 1);
		}


		obfstring trimleft(const obfstring& str)
		{
			return str.length() == 0 ? str : str.substr(str.find_first_not_of(" \t"));
		}


		obfstring trim(const obfstring& str)
		{
			return str.length() == 0 ? str : trimleft(trimright(str));
		}


		std::wstring lower(const std::wstring& str)
		{
			std::wstring tmp = str;
			std::transform(tmp.begin(), tmp.end(), tmp.begin(), tolower);

			return tmp;
		}


		std::string lower(const std::string& str)
		{
			std::string tmp = str;
			std::transform(tmp.begin(), tmp.end(), tmp.begin(), tolower);

			return tmp;
		}


		std::wstring upper(const std::wstring& str)
		{
			std::wstring tmp = str;
			std::transform(tmp.begin(), tmp.end(), tmp.begin(), toupper);

			return tmp;
		}


		std::string upper(const std::string& str)
		{
			std::string tmp = str;
			std::transform(tmp.begin(), tmp.end(), tmp.begin(), toupper);

			return tmp;
		}


		void serase(std::string& str)
		{
			for (char& c : str)
				c = ' ';
		}


		void serase(std::wstring& str)
		{
			for (wchar_t& c : str)
				c = ' ';
		}


		std::wstring substvar(const std::wstring& str, const strimap& vars)
		{
			std::wstring res = str;

			if (vars.size() > 0) {
				bool finish = false;

				while (!finish) {
					std::wstring::size_type end_var = 0;
					std::wstring::size_type pos_var = res.find(L"${");

					if (pos_var == std::wstring::npos) {
						finish = true;

					}
					else {
						std::wstring sb;

						sb.append(res.substr(0, pos_var));
						pos_var += 2;

						end_var = res.find(L"}", pos_var);

						if (end_var != std::string::npos) {
							std::string var = wstr2str(res.substr(pos_var, end_var - pos_var));
							if (vars.find(var) != vars.end()) {
								sb.append(str2wstr(vars.at(var)));
							}

							if (end_var + 1 < res.length()) {
								sb.append(res.substr(end_var + 1));
							}
						}

						res = sb;
					}
				}
			}

			return res;
		}


		std::wstring quote(const std::wstring& str)
		{
			std::wstring quoted_str;

			if (str.find(' ', 0) == std::wstring::npos && str.find('\t', 0) == std::wstring::npos) {
				// no need to quote
				quoted_str = str;

			}
			else {
				quoted_str.push_back('"');

				for (const wchar_t& c : str) {
					if (c == '"') {
						quoted_str.push_back('\\');
						quoted_str.push_back(c);
					}
					else {
						quoted_str.push_back(c);
					}
				}

				quoted_str.push_back('"');
			}

			return quoted_str;
		}


		std::string string_format(const char* fmt, ...)
		{
			std::string result;

			va_list args;
			va_start(args, fmt);
			result = string_format(fmt, args);
			va_end(args);

			return result;
		}


		std::string string_format(const char* fmt, va_list args)
		{
			size_t buffer_size = 256;

			while (true)
			{
				va_list args_copy;
				std::vector<char> buffer(buffer_size);

				va_copy(args_copy, args);
				const int n = std::vsnprintf(buffer.data(), buffer_size, fmt, args_copy);
				va_end(args_copy);

				if (n < 0)
					return "vsnprintf format error";

				// The string has been completely written only when n is non negative 
				// and less than size.  The buffer contains a null terminated string.
				if (n > 0 && n < buffer_size)
					return buffer.data();
				else
					buffer_size *= 2;
			}
		}


		std::string wstr2str(const std::wstring& wstr)
		{
			if (wstr.empty()) return {};

			const int size = ::WideCharToMultiByte(
				CP_UTF8, 0,
				wstr.data(), (int)wstr.size(),
				nullptr, 0,
				nullptr, nullptr
			);

			std::vector<char> result(size, 0);

			::WideCharToMultiByte(
				CP_UTF8, 0,
				wstr.data(), (int)wstr.size(),
				result.data(), (int)result.size(),
				nullptr, nullptr
			);

			return std::string(result.data(), result.size());
		}


		std::wstring str2wstr(const std::string& str)
		{
			const int size = ::MultiByteToWideChar(
				CP_UTF8, 0,
				str.data(), (int)str.size(),
				nullptr, 0);
			std::vector<wchar_t> result(size, 0);

			::MultiByteToWideChar(CP_UTF8, 0,
				str.data(), (int)str.size(),
				result.data(), (int)result.size());

			return std::wstring(result.data(), result.size());
		}
	}
}
