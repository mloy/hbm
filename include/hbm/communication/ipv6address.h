// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#ifndef _IPV6ADDRESS_T_H
#define _IPV6ADDRESS_T_H


#include <string>

namespace hbm {
	namespace communication {
		struct ipv6Address_t {
			bool equal(const ipv6Address_t &op) const;

			std::string address;
			unsigned int prefix;
		};
	}
}
#endif
