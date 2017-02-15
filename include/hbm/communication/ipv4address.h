// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#ifndef _IPV4ADDRESS_T_H
#define _IPV4ADDRESS_T_H


#include <string>

namespace hbm {
	namespace communication {
		struct ipv4Address_t {
			bool equal(const struct ipv4Address_t& op) const;

			std::string getSubnet() const;

			std::string address;
			std::string netmask;
		};
	}
}
#endif
