// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <hbm/string/split.h>


#include "hbm/communication/ipv6Address_t.h"



namespace hbm {
	namespace communication {
		bool ipv6Address_t::equal(const struct ipv6Address_t& op) const
		{
			if( (address==op.address) && prefix==op.prefix) {
				return true;
			}
			return false;
		}
	}
}

