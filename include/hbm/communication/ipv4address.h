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

			static bool isApipaAddress(const std::string& address);
			static bool isValidManualAddress(const std::string& ip);
			static bool isValidNetmask(const std::string& ip);

			/// gaps are not allowed!
			static int getPrefixFromNetmask(const std::string& netmask);
			static std::string getNetmaskFromPrefix(unsigned int prefix);

			std::string address;
			std::string netmask;
		};
	}
}
#endif
