/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Counters.h"


namespace tools {

	void Counters::clear() noexcept
	{
		sent = 0;
		received = 0;
	}


	bool Counters::operator==(const Counters& other) const noexcept
	{
		return (this->sent == other.sent) && (this->received == other.received);
	}


	bool Counters::operator!=(const Counters & other) const noexcept
	{
		return (this->sent != other.sent) || (this->received != other.received);
	}

}
