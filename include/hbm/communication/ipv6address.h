// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#ifndef _IPV6ADDRESS_T_H
#define _IPV6ADDRESS_T_H


#include <string>

namespace hbm {
	namespace communication {
		struct Ipv6Address {
			Ipv6Address();
			bool equal(const Ipv6Address &op) const;

			static bool isLinkLocalAddress(const std::string& address);
			/// returns an empty string if address is not a valid ipv4 mapped ipv6 address
			static std::string getIpv4MappedAddress(const std::string& address);

			std::string address;
			unsigned int prefix;
		};
	}
}
#endif
