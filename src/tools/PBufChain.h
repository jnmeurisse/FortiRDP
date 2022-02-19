/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <list>
#include "lwip/pbuf.h"

namespace tools {
	class PBufChain
	{
		typedef unsigned char byte;

	public:
		explicit PBufChain(struct pbuf* data);
		PBufChain(const PBufChain& p) = delete;
		~PBufChain();

		/* Returns the total number of bytes remaining in the chain
		*/
		inline u16_t len() const noexcept { return _tot_len; }

		/* Returns the pointer to the next bytes in one pbuf
		*/
		inline const byte* cbegin() const noexcept { return _current.payload; }

		/* Returns the pointer to the next-to-the-last byte in the pbuf
		*/
		inline const byte* cend() const noexcept { return _current.payload + _current.len; }

		/* Moves the pointer from the specified length
		*/
		bool move(int len) noexcept;

	private:
		// A pointer to the first buffer, it is the head of a chain of pbufs
		struct pbuf* const _buffer;

		// The total length of the buffer chain.
		u16_t _tot_len;

		// Pointer to the next contiguous block of data in a pbuf.
		struct {
			struct pbuf *pbuf;			// current pbuf in the chain
			byte* payload;				// pointer to the payload in the current pbuf
			u16_t len;					// length of the remaining data in the current pbuf
		} _current;
	};
}