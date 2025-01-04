/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "PBufChain.h"


namespace tools {

	PBufChain::PBufChain() :
		_buffer(nullptr),
		_tot_len(0),
		_current()
	{
	}


	PBufChain::PBufChain(struct pbuf* data) :
		_buffer(data),
		_tot_len(0),
		_current()
	{
		if (data) {
			// we keep a reference to this pbuf
			pbuf_ref(data);

			// assign the current pointer
			_current.pbuf = _buffer;
			_current.payload = (byte *)_buffer->payload;
			_current.len = _buffer->len;

			_tot_len = data->tot_len;
		}
	}


	PBufChain::~PBufChain()
	{
		if (_buffer) {
			pbuf_free(_buffer);
		}
	}

	
	bool PBufChain::move(int len) noexcept
	{
		if (_tot_len - len < 0)
			return false;

		// report what was effectively sent
		_tot_len -= len;

		// move our pointer into the payload
		_current.len -= len;

		// it is not allowed to move over multiple pbuf
		if (_current.len < 0)
			return false;

		// move inside the pbuf
		_current.payload = ((byte *)_current.payload + len);
		
		// when at end of the payload, move to the next in the chain
		if (_current.len == 0 && _tot_len > 0) {
			_current.pbuf = _current.pbuf->next;
			_current.payload = (byte *)_current.pbuf->payload;
			_current.len = _current.pbuf->len;
		}

		return true;
	}

}
