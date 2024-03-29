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
			/// \return if ipv6 address and prefix are equal
			bool equal(const Ipv6Address &op) const;

			/// \return if address is a valid ipv6 address with the prefix "fe80::"
			static bool isLinkLocalAddress(const std::string& address);

			/// \return an empty string if address is not a valid ipv4 mapped ipv6 address
			static std::string getIpv4MappedAddress(const std::string& address);

			/// the address as string i.e fe80::999c:a84:2f22:1ccd
			std::string address;
			/// the network prefix (number of bits leftmost desribing the network)
			unsigned int prefix;
		};
	}
}
#endif
