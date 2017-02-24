// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <hbm/string/split.h>


#include "hbm/communication/ipv6address.h"



namespace hbm {
	namespace communication {
		Ipv6Address::Ipv6Address()
			: prefix(0)
		{
		}

		bool Ipv6Address::equal(const struct Ipv6Address& op) const
		{
			if( (address==op.address) && prefix==op.prefix) {
				return true;
			}
			return false;
		}
	}
}

