// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#ifndef _IPV4ADDRESS_T_H
#define _IPV4ADDRESS_T_H


#include <string>

namespace hbm {
	namespace communication {
		/// Tools concerning ipv4 addresses
		struct Ipv4Address {
			bool equal(const struct Ipv4Address& op) const;

			/// \return subnet resulting of address and netmask
			std::string getSubnet() const;

			/// \return if address is in 169.254.0.0/16
			static bool isApipaAddress(const std::string& address);
			/// some address ranges are reserved and may not be used
			/// (see https://en.wikipedia.org/wiki/Reserved_IP_addresses)
			static bool isValidManualAddress(const std::string& ip);
			static bool isValidNetmask(const std::string& ip);

			/// \return prefix corresponding to the given netmaks
			/// gaps are not allowed!
			static int getPrefixFromNetmask(const std::string& netmask);
			
			/// \return netmask corresponding to the given prefix
			static std::string getNetmaskFromPrefix(unsigned int prefix);

			std::string address;
			std::string netmask;
		};
	}
}
#endif
