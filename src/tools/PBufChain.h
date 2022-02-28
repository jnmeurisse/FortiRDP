/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <list>
#include <cstdint>

#include "lwip/pbuf.h"

namespace tools {
	/**
	* A PBufChain is a wrapper around LWIP pbuf chain.  A "pbuf chain" is 
	* a singly linked list of pbuf. pbuf chain are reference counted.
	* The reference counter is incremented when this object class is allocated
	* (see pbuf_ref) and decremented when the object is destroyed (see pbuf_free)
	* 
	* The class provides an iterator to get successive reference of continuous blocks
	* of bytes in a pbuf.  The functions cbegin() is pointing to the beginning
	* and cend() is pointing to the past-the-end byte in a pbuf.  The function 
	* move() is used to move cbegin() inside a pbuf or move cbegin() and 
	* cend() inside the next pbuf in the chain.
	*
	* The typical usage is
	*		while len() > 0
	*			// Do something on a pbuf and returns the number of bytes processed
	*			len = do_something(cbegin(), cend());
	*
	*			// move cbegin inside the same pbuf or to the next pbuf
	*			move(len);
	*
	*  The pbuf chain structures remain untouched. 
	*/
	class PBufChain final
	{
		using byte = uint8_t;

	public:
		/* Allocates an empty pbuf chain
		*/
		PBufChain();

		/* Allocates a PBufChain around a lwIP pbuf chain.  
		*  The ref counter is incremented
		*/
		explicit PBufChain(struct pbuf* data);
		
		/* Copying a pbuf chain is not implemented.  If needed, pay
		*  attention to the ref counter.
		*/
		PBufChain(const PBufChain& p) = delete;
		
		/* Releases the pbuf chain.  If the ref counter drops to 
		*  zero, the pbuf chain is freed from memory.
		*/
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

		/* Moves the pointer from the specified length.  It is not allowed
		*  to move past the end of a pbuf. The specified length must
		*  be lower that cend()-cbegin() and can not be called when len() is <= 0.
		*  The function returns false if the len is not valid.
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