// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <map>

#include <windows.h>
#define syslog fprintf
#define LOG_ERR stderr

#include "hbm/communication/netadapter.h"

namespace hbm {
	namespace communication {
		Netadapter::Netadapter()
			: m_name()
			, m_ipv4Addresses()
			, m_ipv6Addresses()
			, m_macAddress()
			, m_fwGuid(0)
			, m_index()
		{
		}

		std::string Netadapter::getIpv4DefaultGateway()
		{
			return "";
		}

		bool Netadapter::isApipaAddress(const std::string& address)
		{
			static const std::string apipaNet("169.254");
			if(address.find(apipaNet)==0) {
				return true;
			} else {
				return false;
			}
		}

		bool Netadapter::isValidManualIpv4Address(const std::string& ip)
		{
			unsigned long address = inet_addr(ip.c_str());

			if (address == INADDR_NONE) {
				return false;
			}
			uint32_t bigAddress = htonl(address);

			// check for some reserved ranges
			uint8_t upperMost = bigAddress >> 24;
			if (upperMost==0){
				// includes 0.0.0.0
				return false;
			} else if ((upperMost==24)||(upperMost==25)||(upperMost==26)) {
				// reserved for quantumx internal firewire communcation
				return false;
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

		bool Netadapter::isValidIpv4Netmask(const std::string& ip)
		{
			unsigned long address = inet_addr(ip.c_str());

			if (address == INADDR_NONE) {
				return false;
			}

			uint32_t bigAddress = htonl(address);

			if (bigAddress==0){
				// 0.0.0.0
				return false;
			} else if (bigAddress==0xffffffff) {
				// 255.255.255.255
				return false;
			}

			return true;
		}
		
		int Netadapter::getPrefixFromIpv4Netmask(const std::string& netmask)
		{
			unsigned int prefix = 0;
			unsigned int mask = 0x80000000;
			unsigned long addr = inet_addr(netmask.c_str());
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
			return prefix;
		}

		std::string Netadapter::getIpv4NetmaskFromPrefix(unsigned int prefix)
		{
			if (prefix>32) {
				// invalid
				return "";
			}
			unsigned int subnet = 0;
			for (unsigned int count = 0; count<32; ++count) {
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
	}
}



