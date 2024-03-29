// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <algorithm>

#ifdef _WIN32
#ifndef _WINSOCK2API_
#include <ws2tcpip.h>
#endif
#else
#include <sys/types.h>
#include <arpa/inet.h>
#endif


#include "hbm/communication/ipv6address.h"


#ifdef _WIN32
static int inet_pton_forWindowsxp(int af, const char *src, void *dst)
{
	struct sockaddr_storage ss;
	int size = sizeof(ss);
	char src_copy[INET6_ADDRSTRLEN + 1];

	ZeroMemory(&ss, sizeof(ss));
	/* stupid non-const API */
	strncpy(src_copy, src, INET6_ADDRSTRLEN + 1);
	src_copy[INET6_ADDRSTRLEN] = 0;

	if (WSAStringToAddressA(src_copy, af, nullptr, (struct sockaddr *)&ss, &size) == 0) {
		switch (af) {
		case AF_INET:
			*(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
			return 1;
		case AF_INET6:
			*(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
			return 1;
		}
	}
	return 0;
}
#endif


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

			std::string ipv4Address = address.substr(hybridIpv4AddressPrefix.length());
			size_t count = std::count(ipv4Address.begin(), ipv4Address.end(), '.');
			if (count!=3) {
				return "";
			}
#ifdef _WIN32
			unsigned long addr = inet_addr(ipv4Address.c_str());
			if (addr == INADDR_NONE) {
				return "";
			}
#else
			struct in_addr inSubnet;
			if (inet_aton(ipv4Address.c_str(), &inSubnet) == 0) {
				return "";
			}
#endif
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

			std::string prefix = address.substr(0, ipv6LinkLocalNet.length());
			std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::tolower);
			if (prefix!=ipv6LinkLocalNet) {
				return false;
			}
			return true;
		}
	}
}

