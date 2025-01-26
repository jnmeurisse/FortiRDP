/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "OutputQueue.h"


namespace tools {

	OutputQueue::OutputQueue(size_t capacity) :
		_chain_list(),
		_capacity(capacity)
	{
	}


	OutputQueue::~OutputQueue()
	{
		clear();
	}


	void OutputQueue::clear()
	{
		while (!_chain_list.empty()) {
			PBufChain* const p = _chain_list.front();

			_chain_list.pop_front();
			delete p;
		}
	}

	
	int OutputQueue::append(pbuf* data)
	{
		int len;

		if (full()) {
			len = -1;
		}
		else
		{
			PBufChain* const chain = new PBufChain(data);

			if (chain) {
				len = chain->len();
				_chain_list.push_back(chain);
			}
			else {
				len = 0;
			}
		}

		return len;
	}


	size_t OutputQueue::len() const noexcept
	{
		size_t tot_len = 0;

		for (auto chain : _chain_list) {
			tot_len += chain->len();
		}

		return tot_len;
	}

}
