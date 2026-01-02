/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "ObfuscatedString.h"

#include <ctime>


namespace aux {

	obfstring::obfstring():
		_key(obfstring::create_obfuscation_key()),
		_obfuscated_text("")
	{
	}


	obfstring::obfstring(const char* data, size_t len) :
		obfstring()
	{
		append(data, len);
	}


	obfstring::obfstring(uint8_t key, const char* secstr) :
		_key(key),
		_obfuscated_text(secstr)
	{
	}


	obfstring::obfstring(const std::string& str) :
		_key(obfstring::create_obfuscation_key()),
		_obfuscated_text("")
	{
		append(str);
	}


	obfstring::obfstring(obfstring&& other) :
		_key(other._key),
		_obfuscated_text(other._obfuscated_text)
	{
	}


	obfstring& obfstring::append(const char* str, size_t n)
	{
		const size_t old_size = _obfuscated_text.size();

		// Extend the buffer to hold the concatenated string.
		_obfuscated_text.resize(old_size + n, 0x00);

		// Concatenate : obfuscate the string that it is appended.
		char *p = &_obfuscated_text[old_size];
		for (int index = 0; index < n; index++, p++) {
			*p = encode(str[index], _key);
		}

		return *this;
	}

	
	obfstring& obfstring::append(const std::string& str)
	{
		return append(str.c_str(), str.size());
	}


	obfstring& obfstring::append(const obfstring& str)
	{
		const size_t old_size = _obfuscated_text.size();

		// Extend the buffer to hold the concatenated string.
		_obfuscated_text.resize(old_size + str.size(), 0x00);

		// Concatenate : de-obfuscate the given string and obfuscate when appending.
		char *p = &_obfuscated_text[old_size];
		for (int index = 0; index < str.size(); index++, p++) {
			*p = encode(str[index], _key);
		}

		return *this;
	}


	void obfstring::push_back(char c)
	{
		_obfuscated_text.push_back(encode(c, _key));
	}


	void obfstring::clear()
	{
		_key = obfstring::create_obfuscation_key();
		_obfuscated_text.clear();
	}

	
	size_t obfstring::find(const char ch) const
	{	
		return _obfuscated_text.find(encode(ch, _key));
	}


	size_t obfstring::find_last_not_of(const std::string& str) const
	{
		std::string searched;
		for (auto c : str)
			searched.push_back(encode(c, _key));
		return _obfuscated_text.find_last_not_of(searched);
	}


	size_t obfstring::find_first_not_of(const std::string& str) const
	{
		std::string searched;
		for (auto c : str)
			searched.push_back(encode(c, _key));
		return _obfuscated_text.find_first_not_of(searched);
	}

	
	obfstring obfstring::substr(size_t pos, size_t len) const
	{
		return obfstring(_key, _obfuscated_text.substr(pos, len).c_str());
	}

	
	std::string obfstring::uncrypt() const
	{
		std::string plain_text(_obfuscated_text);

		for (auto& c : plain_text)
			c = decode(c, _key);

		return plain_text;
	}


	void obfstring::uncrypt(char* buffer, size_t size, size_t offset) const noexcept
	{
		for (int index = 0; (index < _obfuscated_text.size()) && (index + offset < size); index++)
			buffer[offset + index] = decode(_obfuscated_text[index], _key);
	}


	uint8_t obfstring::create_obfuscation_key() noexcept
	{
		clock_t value = std::clock();
		return (value % 7) + 1;
	}

}
