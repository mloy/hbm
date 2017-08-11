// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>

#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <syslog.h>

#include "hbm/communication/netadapter.h"
#include "hbm/string/split.h"

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
			std::string gatewayString("0.0.0.0");

			FILE* fp = ::fopen("/proc/net/route", "r");

			if (fp != NULL) {
				if (fscanf(fp, "%*[^\n]\n") < 0) { // Skip the first line
					::syslog(LOG_ERR, "error reading first line of /proc/net/route!");
				} else {
					while (1) {
						int result;
						char deviceName[64];
						unsigned long destination, gateway, mask;
						unsigned int flags;
						int refCnt, use, metric, mtu, window, irtt;
						static const int ROUTE_GW = 0x0002;
						static const int ROUTE_UP = 0x0001;

						result = ::fscanf(fp, "%63s%8lx%8lx%8X%8d%8d%8d%8lx%8d%8d%8d\n",
							deviceName, &destination, &gateway, &flags, &refCnt, &use, &metric, &mask,
							&mtu, &window, &irtt);
						if (result != 11) {
							if ((result < 0) && feof(fp)) { /* EOF with no (nonspace) chars read. */
								break;
							}
						}

						if (((flags & ROUTE_GW) != 0) &&  // route is gateway
							((flags & ROUTE_UP) != 0)) { // route is up
								if (destination == INADDR_ANY) { // default gateway
									struct sockaddr_in s_in;
									s_in.sin_addr.s_addr = gateway;
									gatewayString = ::inet_ntoa(s_in.sin_addr);
									break;
								}
						}
					}
				}
				::fclose(fp);
			}
			return gatewayString;
		}

		bool Netadapter::isFirewireAdapter() const
		{
			if (m_fwGuid) {
				return true;
			} else {
				return false;
			}
		}

		

	}
}
