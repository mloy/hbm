// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <algorithm>
#include <stdint.h>

#ifdef _WIN32
#ifndef _WINSOCK2API_
#include <ws2tcpip.h>
#endif
#else
#include <sys/types.h>
#include <arpa/inet.h>
#endif

#include <hbm/string/split.h>

#include "hbm/communication/ipv4address.h"

namespace hbm {
	namespace communication {
		bool Ipv4Address::equal(const struct Ipv4Address& op) const
		{
			if( (address==op.address) && (netmask==op.netmask) ) {
				return true;
			}
			return false;
		}

		bool Ipv4Address::isApipaAddress(const std::string& address)
		{
			static const std::string apipaNet("169.254");

			size_t count = std::count(address.begin(), address.end(), '.');
			if (count!=3) {
				return false;
			}
#ifdef _WIN32
			unsigned long addr = inet_addr(address.c_str());
			if (addr == INADDR_NONE) {
				return false;
			}
#else
			in_addr addr;
			if (inet_aton(address.c_str(), &addr) == 0) {
				return false;
			}
#endif

			if(address.find(apipaNet)==0) {
				return true;
			} else {
				return false;
			}
		}

		bool Ipv4Address::isValidManualAddress(const std::string& ip)
		{
			size_t count = std::count(ip.begin(), ip.end(), '.');
			if (count!=3) {
				return false;
			}

#ifdef _WIN32
			unsigned long addr = inet_addr(ip.c_str());
			if (addr == INADDR_NONE) {
				return false;
			}
			uint32_t bigAddress = htonl(addr);
#else
			in_addr address;
			if (inet_aton(ip.c_str(), &address) == 0) {
				return false;
			}
			uint32_t bigAddress = htonl(address.s_addr);
#endif
			if (bigAddress==0) {
				// "0.0.0.0" is allowed and means "non-routable meta-address"
				return true;
			}

			// check for some reserved ranges
			uint8_t upperMost = bigAddress >> 24;
			if (upperMost==0){
				return false;
// this is specific for QuantumX and should not be handled here!
//			} else if ((upperMost==24)||(upperMost==25)||(upperMost==26)) {
//				// reserved for quantumx internal firewire communcation
//				return false;
			} else if (upperMost==127) {
				// Loopback and diagnostics
				return false;
			} else if (upperMost>=224) {
				// 224 - 239: Reserved for Multicasting
				// 240 - 254: Experimental; used for research
				return false;
			}

			if(isApipaAddress(ip)) {
				return false;
			}

			return true;
		}

		bool Ipv4Address::isValidNetmask(const std::string& ip)
		{
			size_t count = std::count(ip.begin(), ip.end(), '.');
			if (count!=3) {
				return false;
			}

#ifdef _WIN32
			unsigned long addr = inet_addr(ip.c_str());
			if (addr == INADDR_NONE) {
				return false;
			}
			uint32_t bigAddress = htonl(addr);
#else
			in_addr address;
			if (inet_aton(ip.c_str(), &address) == 0) {
				return false;
			}
			uint32_t bigAddress = htonl(address.s_addr);
#endif

			if (bigAddress==0){
				// 0.0.0.0
				return false;
			} else if (bigAddress==0xffffffff) {
				// 255.255.255.255
				return false;
			}

			return true;
		}

		std::string Ipv4Address::getSubnet() const
		{
			std::string subnet;
			unsigned int addressByte;
			unsigned int netmaskByte;
			unsigned int subnetByte;
			hbm::string::tokens addressParts = hbm::string::split(address, '.');
			hbm::string::tokens netmaskParts = hbm::string::split(netmask, '.');
			if ((addressParts.size()!=4)||(netmaskParts.size()!=4)) {
				return "";
			}

			for (unsigned int index=0; index<4; ++index) {
				addressByte = std::stoul(addressParts[index], NULL, 10);
				netmaskByte = std::stoul(netmaskParts[index], NULL, 10);
				if ((addressByte>255) || (netmaskByte>255)) {
					// not a valid address!
					return "";
				}
				subnetByte = addressByte & netmaskByte;
				subnet += std::to_string(subnetByte) + '.';
			}
			subnet.pop_back(); // remove trailing '.'
			return subnet;
		}

		std::string Ipv4Address::getNetmaskFromPrefix(unsigned int prefix)
		{
			if (prefix>32) {
				// invalid
				return "";
			}
			unsigned int subnet = 0;
			for (unsigned int count=0; count<32; ++count) {
				subnet <<= 1;
				if (prefix) {
					subnet |= 1;
					--prefix;
				}
			}

			struct in_addr ip_addr;
			ip_addr.s_addr = htonl(subnet);
			return inet_ntoa(ip_addr);
		}

		int Ipv4Address::getPrefixFromNetmask(const std::string& netmask)
		{
			unsigned int prefix = 0;
			unsigned int mask = 0x80000000;
#ifdef _WIN32
			unsigned int addr = inet_addr(netmask.c_str());
#else
			in_addr_t addr = inet_addr(netmask.c_str());
#endif
			//255.255.255.255 is valid!
			if ((addr==INADDR_NONE) && (netmask!="255.255.255.255")) {
				return -1;
			}
			uint32_t ipv4Subnetmask = ntohl(addr);
			do {
				if (ipv4Subnetmask & mask) {
					mask >>= 1;
					++prefix;
				} else {
					break;
				}
			} while(mask!=0);

			// check for following gaps which are not allowed!
			for(unsigned int pos=prefix+1; pos<32; ++pos) {
				mask >>= 1;
				if (ipv4Subnetmask & mask) {
					return -1;
				}
			}
			return prefix;
		}
	}
}

