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
		_capacity(capacity),
		_used(0),
		_data(new byte[capacity])
	{
		::SecureZeroMemory(_data, _capacity);
	}


	ByteBuffer::~ByteBuffer()
	{
		if (_data) {
			::SecureZeroMemory(_data, _capacity);
			delete[] _data;
		}
	}


	void ByteBuffer::clear() noexcept
	{
		_used = 0;
		::SecureZeroMemory(_data, _capacity);
	}


	void ByteBuffer::resize(size_t capacity)
	{
		const int block = 1024;

		if (capacity > _capacity) {
			// allocate a new buffer 
			capacity = block * ((capacity / block) + 1);
			byte* const ptr = new byte[capacity];
			::SecureZeroMemory(ptr, capacity);

			// copy current content
			std::memcpy(ptr, _data, _used * sizeof(byte));

			// replace the old buffer by the new
			delete[] _data;
			_data = ptr;
			_capacity = capacity;
		}
	}


	ByteBuffer& ByteBuffer::append(const void* data, size_t size)
	{
		if (size > 0) {
			// make sure we have enough space
			resize(_used + size);

			// append the data at the end of the buffer
			std::memcpy(_data + _used, data, size * sizeof(byte));

			_used += size;
		}

		return *this;
	}


	ByteBuffer& ByteBuffer::append(const std::string& data)
	{
		return append(data.c_str(), data.size());
	}


	ByteBuffer& ByteBuffer::append(const tools::obfstring &data)
	{
		const size_t size = data.size();

		if (size > 0) {
			// make sure we have enough space
			resize(_used + size);

			// append the decrypted data at the end of the buffer
			data.uncrypt((char *)_data, _capacity, _used);

			_used += size;
		}

		return *this;
	}
}