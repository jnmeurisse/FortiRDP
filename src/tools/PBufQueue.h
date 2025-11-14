/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <cstdint>
#include <lwip/pbuf.h>


namespace tools {

	/**
	 * PBufQueue represents a queue of LWIP pbufs (packet buffers). The queue is
	 * implemented as a "pbuf chain", a singly linked list of pbufs where each
	 * pbuf is reference counted. The reference counter is incremented when the
	 * queue takes ownership and decremented when a pbuf is removed. The total
	 * size of a pbuf chain is limited to 2^16-1 bytes.
	 *
	 * This class provides an iterator-like interface for accessing successive
	 * contiguous blocks of bytes within a pbuf. A `cblock` represents a
	 * contiguous block and contains:
	 *   - A pointer to the first byte of the block.
	 *   - The length of the block.
	 *   - A flag indicating whether more data is available in the current pbuf
	 *     or if the next pbuf in the queue was pushed without the
	 *     `PBUF_FLAG_PUSH` flag.
	 *
	 * The function `get_cblock()` returns the current contiguous block, while
	 * `get_cblock(len)` returns a block with a size not exceeding `len`.
	 *
	 * The `move()` function advances the current contiguous block within a
	 * single pbuf or transitions to the next block in the queue.
	 *
	 *
	 * Typical usage:
	 *
	 *   while (!is_empty()) {
	 *       // Get the next block
	 *       cblock block = get_cblock();
	 *
	 *       // Process a block of data and return the number of bytes processed
	 *       size_t len = do_something(block);
	 *
	 *       // Advance the iterator within the queue to the next block
	 *       move(len);
	 *   }
	 */
	class PBufQueue
	{
		using byte = uint8_t;

	public:
		/**
		 * Constructs a PBufQueue with a specified capacity.
		 * 
		 * @param capacity The requested capacity for the queue (bytes)
		 */
		explicit PBufQueue(uint16_t capacity);

		/**
		*  Copying a pbuf queue is not implemented.
		*/
		PBufQueue(const PBufQueue& p) = delete;
		
		/**
		 * Releases the queue chain.
		*/
		~PBufQueue();

		/**
		 * Clears the queue.
		*/
		void clear();

		/**
		 * Appends a pbuf or a chain of pbufs to the end of the queue.
		 *
		 * @param buffer Pointer to the `pbuf` structure to be appended.
		 *
		 * @return true if the `pbuf` was successfully appended to the queue;
		 *         false if the `buffer` is null, empty or if appending it would
		 *         exceed the queue's capacity.
		 *
		 * Note : the given buffer is now referenced by this queue.  The caller can
		 *        free the buffer (using pbuf_free) if it does not use it anymore.
		 *
		 */
		bool push(struct pbuf* buffer);

		/**
		* Removes the first pbuf from the queue.
		* 
		* @return a pointer to the pbuf removed from the queue or a null pointer
		*         if the queue was empty.  The caller becomes the owner of the
		*         pbuf and is responsible to free it.
		*/
		struct pbuf* pop();

		/**
		* Returns the total number of bytes occupied in the queue.
		*/
		inline size_t size() const noexcept;

		/**
		* Returns the remaining space available in the queue.
		* 
		* Note : The function returns capacity - size.  If the remaining space is
		* > 0, it is still possible to push another pbuf in the queue having a tot_len
		* less than the remaining space.
		*/
		inline size_t remaining_space() const noexcept;

		/**
		* Returns the number of pbuf chained in the queue.
		*/
		uint16_t count() const noexcept;

		/**
		* Returns true if the queue is empty.
		*/
		inline bool is_empty() const noexcept { return !_chain; }

		/**
		* Returns true if the queue is full.
		*/
		inline bool is_full() const noexcept { return remaining_space() == 0; }

		/* A contiguous block of data in the queue.
		*/
		struct cblock {
			const byte* pdata;  // pointer to the first byte in the pbuf payload
			const uint16_t len; // length of the block
			const bool more;	// is there more data
		};

		/**
		* Returns the first continuous block of data with a size not exceeding `len`.
		*/
		cblock get_cblock(size_t len) const;

		/**
		* Returns the first largest continuous block of data.
		*/
		cblock get_cblock() const;

		/**
		 * Moves the read offset within the payload of the current `pbuf` in the queue.
		 *
		 * @param len The number of bytes to move the offset forward.
		 *
		 * @return true if the offset was successfully moved; false if the queue
		 *         is empty or if the move would exceed the length of the current `pbuf`.
		 *
		 * When the function detects that the first pbuf in the chain is fully consumed,
		 * it release the reference to the buffer (and free the memory when the reference
		 * counter drops to zero) and moves the internal pointer to the next pbuf in 
		 * the chain.
		*/
		bool move(size_t len) noexcept;

	private:
		// The capacity of this queue.
		const size_t _capacity;

		// A pointer to the first buffer, it is the head of a chain of pbufs.
		// The pointer is null when the queue is empty.
		struct pbuf* _chain;

		// An offset in the head of the chain.
		size_t _offset;

		// A convenient function that returns a pointer to the payload.
		inline const byte* payload() const noexcept { return static_cast<byte*>(_chain->payload); }

		// A convenient function that returns the pbuf len as a size_t
		static inline size_t pbuf_len(const struct pbuf* buffer);

		// A convenient function that returns the pbuf tot_len as a size_t
		static inline size_t pbuf_tot_len(const struct pbuf* buffer);
	};

}
