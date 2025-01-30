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
		if (!buffer || buffer->tot_len == 0 || is_full())
			return false;

		if (_chain) {
			// The queue is not empty, we calculate if the new total length would
			// not exceed the queue capacity.
			if (size() + buf_tot_len(buffer) > _capacity)
				return false;

			// append the pbuf at the end of the queue and add a reference to it.
			pbuf_chain(_chain, buffer);
		}
		else {
			// The chain is empty.  The given buffer is now the head of the chain.
			_chain = buffer;
			_offset = 0;

			// we keep a reference to this pbuf.
			pbuf_ref(buffer);
		}

		return true;
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

	
	bool PBufQueue::move(size_t len) noexcept
	{
		if (!_chain)
			return false;

		// it is not allowed to move past the end of a pbuf
		if (_offset + len > buf_len(_chain))
			return false;

		// move the offset pointer into the payload
		_offset += len;
		if (buf_len(_chain) - _offset == 0) 
			pbuf_free(pop());
	
		return true;
	}


	size_t PBufQueue::buf_len(const pbuf* buffer)
	{
		return static_cast<size_t>(buffer->len);
	}


	size_t PBufQueue::buf_tot_len(const pbuf* buffer)
	{
		return static_cast<size_t>(buffer->tot_len);
	}

}
