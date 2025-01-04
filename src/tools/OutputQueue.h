/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <list>
#include <lwip/pbuf.h>
#include "tools/PBufChain.h"


namespace tools {

	class OutputQueue
	{
	public:
		/* Allocates a new queue having the specified capacity.  
		*
		* @param capacity The queue's capacity
		*/
		explicit OutputQueue(int capacity);

		/* Destroy this buffer
		*/
		virtual ~OutputQueue();

		/* Forbid copying the output queue
		*/
		OutputQueue(const OutputQueue&) = delete;
		OutputQueue& operator=(const OutputQueue&) = delete;

		/* Clears the content of the queue.
		*/
		void clear();

		/* Appends a pbuf chain to the queue.
		*
		* The function returns the number of bytes added to the queue or -1
		* if the queue is over its capacity. 
		*/
		int append(pbuf* data);

		/* Returns the queue's capacity
		*/
		inline int capacity() const noexcept { return _capacity; }

		/* Returns the number of bytes remaining in the queue
		*/
		int len() const noexcept;

		/* Returns true if queue is over its capacity
		*/
		inline bool full() const noexcept { return _chain_list.size() > capacity(); }

		/* Returns true if the queue is empty
		*/
		inline bool empty() const noexcept { return _chain_list.empty(); }

		/* Returns the first PBufChain in the queue.
		*/
		inline PBufChain* front() const { return _chain_list.front(); }

		/* Deletes the first PBufChain in the queue
		*/
		void pop_front() { return _chain_list.pop_front(); }

	private:
		// A list of pbuf chains
		std::list<PBufChain*> _chain_list;

		// Capacity of the buffer
		const int _capacity;
	};

}
