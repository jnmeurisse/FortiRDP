/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "StrUtil.h"

#include <algorithm>
#include <cctype>
#include <stdlib.h>
#include <codecvt>
#include <cstdarg>
#include <stdio.h>
#include <vadefs.h>


namespace tools {

	int split(const char* str, const char delim, std::vector<std::string>& parts) 
	{
		const size_t count = parts.size();

		do {
			const char* const begin = str;

			while(*str != delim && *str)
				str++;

			parts.push_back(std::string(begin, str));
		} while (0 != *str++);

		return (int) (parts.size() - count);
	}


	int split(const std::string& str, const char delim, std::vector<std::string>& parts)
	{
		return split(str.c_str(), delim, parts);
	}


	int split(const wchar_t* str, const wchar_t delim, std::vector<std::wstring>& parts)
	{
		const size_t count = parts.size();

		do {
			const wchar_t* const begin = str;

			while (*str != delim && *str)
				str++;

			parts.push_back(std::wstring(begin, str));
		} while (0 != *str++);

		return (int)(parts.size() - count);
	}


	int split(const std::wstring& str, const wchar_t delim, std::vector<std::wstring>& parts)
	{
		return split(str.c_str(), delim, parts);
	}


	int split(const obfstring& str, const char delim, std::vector<obfstring>& parts)
	{
		const size_t count = parts.size();

		int index = 0;
		do {
			const int begin = index;
			while(index < str.size() && str[index] != delim)
				index++;

			parts.push_back(str.substr(begin, index-begin));
			index++;
		} while (index < str.size());

		return (int)(parts.size() - count);
	}


	bool str2num(const std::string& numstr, const int radix, const long minval, const long maxval, long &value)
	{
		if (minval > maxval) {
			errno = EINVAL;

		} else {
			errno = 0;
			long l = std::strtol(numstr.c_str(), nullptr, radix);
		
			if (errno == 0) {
				if (l < minval || l > maxval) {
					errno = ERANGE;
				} else {
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


	std::string trim(const std::string & str)
	{
		return str.length() == 0 ? str : trimleft(trimright(str));
	}


	obfstring trimright(const obfstring& str)
	{
		return str.length() == 0 ? str : str.substr(0, str.find_last_not_of(" \t") + 1);
	}


	obfstring trimleft(const obfstring &str)
	{
		return str.length() == 0 ? str : str.substr(str.find_first_not_of(" \t"));
	}


	obfstring trim(const obfstring & str)
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
		for (unsigned int idx = 0; idx < str.length(); idx++)
			str[idx] = ' ';
	}


	void serase(std::wstring & str)
	{
		for (unsigned int idx = 0; idx < str.length(); idx++)
			str[idx] = ' ';
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

				} else {
					std::wstring sb;

					sb.append(res.substr(0, pos_var));
					pos_var += 2;

					end_var = res.find(L"}", pos_var);

					if (end_var != std::string::npos) {
						std::string var = wstr2str(res.substr(pos_var, end_var - pos_var));
						if (vars.find(var) != vars.end()) {
							sb.append(str2wstr(vars.at(var)));
						}

						if (end_var+1 < res.length()) {
							sb.append(res.substr(end_var+1));
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

		} else {
			quoted_str.push_back('"');

			for (const wchar_t &c : str) {
				switch (c) {
				case '"':
					quoted_str.push_back('\\');
					quoted_str.push_back(c);
					break;

				default:
					quoted_str.push_back(c);
					break;
				}
			}

			quoted_str.push_back('"');
		}

		return quoted_str;
	}


	std::string string_format(const std::string fmt, ...) 
	{
		int n, size=100;
		bool b=false;
		va_list marker;
		std::string s;

		while (!b)
		{
			s.resize(size);
			va_start(marker, fmt);
			n = vsnprintf_s((char*)s.c_str(), size, _TRUNCATE, fmt.c_str(), marker);
			va_end(marker);
			if ((n > 0) && ((b=(n < size)))) 
				s.resize(n);
			else 
				size*=2;
		}

		return s;
	}


	std::string wstr2str(const std::wstring& wstr)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.to_bytes(wstr);
	}


	std::string wstr2str(const wchar_t* pwstr)
	{
		return pwstr ? wstr2str(std::wstring(pwstr)) : "";
	}


	std::wstring str2wstr(const std::string& str)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.from_bytes(str);
	}


	std::wstring str2wstr(const char* pstr)
	{
		return pstr ? str2wstr(std::string(pstr)) : L"";
	}

}
