// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <sys/types.h>
#include <arpa/inet.h>

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

		std::string Ipv6Address::getIpv4MappedAddress(const std::string& address)
		{
			static const std::string hybridIpv4AddressPrefix = "::ffff:";
			std::string addressPrefix = address.substr(0, hybridIpv4AddressPrefix.length());

			if (addressPrefix!=hybridIpv4AddressPrefix) {
				return "";
			}

			struct in_addr inSubnet;
			std::string ipv4Address = address.substr(hybridIpv4AddressPrefix.length());
			hbm::string::tokens tokens = hbm::string::split(ipv4Address, '.');
			if (tokens.size()!=4) {
				return "";
			}

			if (inet_aton(ipv4Address.c_str(), &inSubnet)!=1) {
				return "";
			}
			return ipv4Address;
		}

		bool Ipv6Address::isLinkLocalAddress(const std::string& address)
		{
			static const std::string ipv6LinkLocalNet("fe80::");
			struct in6_addr addr;

#ifdef _WIN32
			if (inet_pton_forWindowsxp(AF_INET6, address.c_str(), &addr) == 0) {
				return false;
			}
#else
			if (inet_pton(AF_INET6, address.c_str(), &addr) == 0) {
				return false;
			}
#endif

			if(address.find(ipv6LinkLocalNet)==0) {
				return true;
			} else {
				return false;
			}
		}
	}
}

