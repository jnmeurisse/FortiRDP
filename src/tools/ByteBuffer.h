/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once
#include <string>
#include "tools/ObfuscatedString.h"

namespace tools {
	/**
	* A byte buffer holds a sequence of bytes. This class provides
	* methods to append bytes at the end of the buffer.
	*
	*/
	class ByteBuffer
	{
	public:
		using byte = unsigned char;

		/* Avoid allocating zero capacity buffer
		*/
		ByteBuffer() = delete;

		/* Allocates a new byte buffer having the specified initial capacity.
		 *
		 * @param capacity The new buffer's capacity, in bytes
		*/
		explicit ByteBuffer(size_t capacity);

		/* Avoid copying the buffer.
		*/
		ByteBuffer(const ByteBuffer& buffer) = delete;

		/* Destroy this buffer
		*/
		~ByteBuffer();

		/* Clears this buffer.
		 *
		 * The size of the buffer is set to zero, the capacity is not
		 * updated nor reduced. The method clears the data in the buffer.
		*/
		void clear() noexcept;

		/* Adjusts the capacity of the buffer
		 *
		 * @param capacity The new capacity of the buffer
		 *
		 * This method can only be used to increase the capacity of the buffer.
		 * Trying to set a smaller capacity has no effect on the existing buffer.
		 */
		void resize(size_t capacity);

		/* Appends the given data buffer to the end of this buffer
		 *
		 * @param data	A pointer to the data buffer to append
		 * @param size	The number of bytes to copy at the end of the stream
		 *
		 * @return this ByteBuffer
		*/
		ByteBuffer& append(const void* data, size_t size);

		/* Appends the string to the end of this buffer
		 *
		 * @param data	A string
		 *
		 * \return this ByteBuffer
		*/
		ByteBuffer& append(const std::string& data);

		/* Appends the obfuscated string to the end of this buffer 
		*
		* @param data	A string
		*
		* @return this ByteBuffer
		*/
		ByteBuffer& append(const tools::obfstring& data);

		/* Returns true if the data buffer is empty
		*/
		inline bool empty() const noexcept { return _used == 0; }

		/* Returns the number of used bytes in the buffer
		*/
		inline size_t size() const noexcept { return _used; }

		/* Returns the pointer to the first byte in the buffer
		*/
		inline const byte* cbegin() const noexcept { return _data; }

		/* Returns the pointer to the next-to-the-last byte in the buffer
		*/
		inline const byte* cend() const noexcept { return _data + _used; }

	private:
		// A pointer to the buffer
		byte* _data;

		// Size of the allocated memory
		size_t _capacity;

		// Size of the used memory (always <= _capacity)
		size_t _used;
	};
}