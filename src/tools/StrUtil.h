/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <cstring>
#include <string>
#include <vector>
#include <map>
#include "tools/ObfuscatedString.h"


namespace priv {

struct comp { 
    bool operator() (const std::string& lhs, const std::string& rhs) const {
        return _stricmp(lhs.c_str(), rhs.c_str()) < 0;
    }
};
}

namespace tools {

typedef std::map<const std::string, std::string, priv::comp> strimap;

// Splits a string into multiple parts which are separated by a delimiter. The function
// adds the parts to the specified vector and returns the number of added parts.
size_t split(const char* str, const char delim, std::vector<std::string>& parts);
size_t split(const wchar_t* str, const wchar_t delim, std::vector<std::wstring>& parts);
size_t split(const std::string& str, const char delim, std::vector<std::string>& parts);
size_t split(const std::wstring& str, const wchar_t delim, std::vector<std::wstring>& parts);
size_t split(const obfstring& str, const char delim, std::vector<obfstring>& parts);

// Performs a case insensitive string comparison 
bool iequal(std::string const& s1, std::string const& s2);

// Converts a string to an integer. The function returns true if the conversion
// succeeds. The value parameter remains untouched if an error was detected.
bool str2num(const std::string& numstr, const int radix, const long minval, const long maxval, long &value);
bool str2i(const std::string& numstr, int& value);
bool str2i(const std::wstring& numstr, int& value);

// Trims string 
std::wstring trimright(const std::wstring& str);
std::wstring trimleft(const std::wstring& str);
std::wstring trim(const std::wstring& str);

std::string trimright(const std::string& str);
std::string trimleft(const std::string& str);
std::string trim(const std::string& str);

obfstring trimright(const obfstring& str);
obfstring trimleft(const obfstring& str);
obfstring trim(const obfstring& str);

// Converts string to lower case
std::wstring lower(const std::wstring& str);
std::string lower(const std::string& str);

// Converts string to upper case
std::wstring upper(const std::wstring& str);
std::string upper(const std::string& str);

// "secure" erase the string (replace all characters by a space)
void serase(std::string& str);
void serase(std::wstring& str);

// Performs variables substitution 
std::wstring substvar(const std::wstring& str, const strimap& vars);

// Quote a string (add quote at begin/end of the specified string)
std::wstring quote(const std::wstring& str);

// Formats a string
std::string string_format(const char* fmt, ...);

// Converts wstring to string
std::string wstr2str(const std::wstring& wstr);
std::string wstr2str(const wchar_t* pwstr);

// Converts string to wstring
std::wstring str2wstr(const std::string& str);
std::wstring str2wstr(const char* pstr);
std::wstring str2wstr(const std::u8string& str);

}
