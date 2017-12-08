// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#ifndef _IPV6ADDRESS_T_H
#define _IPV6ADDRESS_T_H


#include <string>

namespace hbm {
	namespace communication {
		/// Tools concerning ipv4 addresses
		struct Ipv6Address {
			Ipv6Address();
			bool equal(const Ipv6Address &op) const;

			/// \return if address is a valid ipv6 address with the prefix "fe80::"
			static bool isLinkLocalAddress(const std::string& address);

			/// \return an empty string if address is not a valid ipv4 mapped ipv6 address
			static std::string getIpv4MappedAddress(const std::string& address);

			std::string address;
			unsigned int prefix;
		};
	}
}
#endif
