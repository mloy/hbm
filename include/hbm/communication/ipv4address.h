// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#ifndef _IPV4ADDRESS_T_H
#define _IPV4ADDRESS_T_H


#include <string>

namespace hbm {
	namespace communication {
		struct Ipv4Address {
			bool equal(const struct Ipv4Address& op) const;

			std::string getSubnet() const;

			std::string address;
			std::string netmask;
		};
	}
}
#endif
