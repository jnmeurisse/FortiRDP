/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "PBufQueue.h"

#include <algorithm>
#include <limits>



namespace tools {

	PBufQueue::PBufQueue(uint16_t capacity) :
		_capacity{ capacity },
		_chain{ nullptr },
		_offset{ 0 }
	{
	}


	PBufQueue::~PBufQueue()
	{
		clear();
	}


	void PBufQueue::clear()
	{
		if (_chain) {
			pbuf_free(_chain);

			_chain = nullptr;
			_offset = 0;
		}
	}


	bool PBufQueue::push(struct pbuf* buffer)
	{
		bool rc = false;

		if (buffer && buffer->tot_len > 0 && !is_full()) {
			if (_chain) {
				// Since the queue is not empty, verify that the total length
				// after adding the new data does not exceed the queue's maximum capacity.
				if (size() + pbuf_tot_len(buffer) <= _capacity) {
					// Append the buffer at the end of the queue.  The queues now references
					// the buffer.
					pbuf_chain(_chain, buffer);
					rc = true;
				}
			}
			else {
				// The chain is empty.  The given buffer is now the head of the chain.
				_chain = buffer;
				_offset = 0;

				// We keep a reference to this buffer.
				pbuf_ref(buffer);
				rc = true;
			}
		}

		return rc;
	}


	struct pbuf* PBufQueue::pop()
	{
		struct pbuf* head = _chain;

		if (_chain) {
			_chain = _chain->next;
			_offset = 0;

			head->tot_len = head->len;
			head->next = nullptr;
		}

		return head;
	}


	inline size_t PBufQueue::size() const noexcept
	{
		return _chain ? _chain->tot_len : 0;
	}


	inline size_t PBufQueue::remaining_space() const noexcept
	{
		return _capacity - size();
	}


	uint16_t PBufQueue::count() const noexcept
	{
		return pbuf_clen(_chain);
	}


	PBufQueue::cblock PBufQueue::get_cblock(size_t len) const
	{
		if (is_empty()) {
			return { nullptr, 0, false };
		}
		else {
			// Compute how many bytes are available in the pbuf.
			size_t available = pbuf_len(_chain) - _offset;

			// Determine if more data is available.
			bool more_data =
				(len < available) ||
				(
					(_chain->flags && PBUF_FLAG_PUSH) == 0 && 
					_chain->next && (_chain->next->flags && PBUF_FLAG_PUSH) == 0
				);

			// The length of the cblock is limited by the amount of data in a single pbuf
			// and has a maximum value of 2^16.
			size_t cblock_len = std::min(len, available);

			// Return the next contiguous block of data
			return {
				payload() + _offset,
				static_cast<uint16_t>(cblock_len),
				more_data
			};
		}
	}


	PBufQueue::cblock PBufQueue::get_cblock() const
	{
		return get_cblock(pbuf_len(_chain) - _offset);
	}

	
	bool PBufQueue::move(size_t len) noexcept
	{
		bool rc = false;

		// Verify that we do not move past the end of a pbuf.
		if (_chain && _offset + len <= pbuf_len(_chain)) {
			// Move the offset pointer into the payload
			_offset += len;

			// Pop the head of the queue if the offset moved at the end of the payload.
			if (pbuf_len(_chain) - _offset == 0)
				pbuf_free(pop());

			rc = true;
		}
	
		return rc;
	}


	size_t PBufQueue::pbuf_len(const pbuf* buffer)
	{
		return static_cast<size_t>(buffer->len);
	}


	size_t PBufQueue::pbuf_tot_len(const pbuf* buffer)
	{
		return static_cast<size_t>(buffer->tot_len);
	}

}
