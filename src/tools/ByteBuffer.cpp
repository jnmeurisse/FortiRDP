/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <Windows.h>
#include "ByteBuffer.h"


namespace tools {

	ByteBuffer::ByteBuffer(size_t capacity) :
		_buffer()
	{
		_buffer.reserve(capacity);
		::SecureZeroMemory(_buffer.data(), _buffer.capacity());
	}


	ByteBuffer::~ByteBuffer()
	{
		::SecureZeroMemory(_buffer.data(), _buffer.capacity());
	}


	void ByteBuffer::clear() noexcept
	{
		::SecureZeroMemory(_buffer.data(), _buffer.capacity());
		_buffer.clear();
	}


	void ByteBuffer::reserve(size_t capacity)
	{
		_buffer.reserve(capacity);
	}


	ByteBuffer& ByteBuffer::append(const byte* data, size_t size)
	{
		_buffer.insert(_buffer.end(), data, data + size);

		return *this;
	}


	ByteBuffer& ByteBuffer::append(const std::string& data)
	{
		return append((const byte*)data.c_str(), data.size());
	}


	ByteBuffer& ByteBuffer::append(const tools::obfstring &data)
	{
		const size_t current_size = _buffer.size();

		// make sure we have enough space
		_buffer.resize(_buffer.size() + data.size());

		// append the decrypted data at the end of the buffer
		data.uncrypt((char *)_buffer.data(), _buffer.capacity(), current_size);

		return *this;
	}
}