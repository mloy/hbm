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

#ifndef _WINSOCK2API_
#include <ws2tcpip.h>
#endif

#define syslog fprintf
#define LOG_ERR stderr

#include "hbm/communication/netadapter.h"
#include "hbm/communication/ipv4address.h"
#include "hbm/communication/wmi.h"


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

		bool Netadapter::isFirewireAdapter() const
		{
			return hbm::communication::WMI::isFirewireAdapter(*this);
		}
	}
}



