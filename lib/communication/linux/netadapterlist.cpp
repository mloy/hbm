// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <stdint.h>
#include <iterator>
#include <mutex>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <iphlpapi.h>
#define syslog fprintf
#define LOG_ERR stderr
#else
#include <unistd.h>
#include <syslog.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

#include <sys/ioctl.h>
#endif

#include "hbm/exception/exception.hpp"

#include "hbm/communication/netadapterlist.h"
#include "hbm/communication/netadapter.h"

namespace hbm {
	namespace communication {
		NetadapterList::NetadapterList()
		{
			update();
		}


		struct hardwareInfo_t {
			hardwareInfo_t()
				: hwAddrString()
				, fwGuid(0)
				, isHardware(false)
			{
			}

			std::string hwAddrString;
			uint64_t fwGuid;
			bool isHardware;
		};

		static hardwareInfo_t getHardwareInfo(const std::string& interfaceName)
		{
			hardwareInfo_t result;
			struct ifreq ifr;
			memset(&ifr, 0, sizeof(ifr));


			int sd =::socket(AF_INET, SOCK_DGRAM, 0);
			if(sd==-1) {
				::syslog(LOG_ERR, "%s: could not create socket!", __FUNCTION__);
			} else {
				::strncpy(ifr.ifr_name, interfaceName.c_str(), sizeof(ifr.ifr_name));
				ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';

				if ((::ioctl(sd, SIOCGIFHWADDR, reinterpret_cast < caddr_t > (&ifr), sizeof(ifr))) == 1) {
					::syslog(LOG_ERR, "%s: error calling ioctl SIOCGIFHWADDR!", __FUNCTION__);
				} else {
					int macLen;

					switch (ifr.ifr_hwaddr.sa_family) {
					case ARPHRD_ETHER:
						macLen = 6;
						result.isHardware = true;
						break;

					case ARPHRD_IEEE1394:
						{
							uint64_t fwGuid = 0;
							macLen = 16;

							for (unsigned int i = 0; i < sizeof(fwGuid); ++i) {
								fwGuid = fwGuid << 8;
								fwGuid |= ifr.ifr_hwaddr.sa_data[i];
							}
							result.isHardware = true;
							result.fwGuid = fwGuid;
						}
						break;

					default:
						macLen = 0;
						break;
					}

					std::stringstream hwAddressStream;

					for (int i = 0; i < macLen; ++i) {
						if (i > 0) {
							hwAddressStream << ":";
						}

						hwAddressStream <<  std::uppercase << std::setfill('0') << std::setw(2) << std::hex <<  static_cast <unsigned int> ( static_cast < unsigned char >(ifr.ifr_hwaddr.sa_data[i]));
					}

					result.hwAddrString = hwAddressStream.str();
				}

				::close(sd);
			}
			return result;
		}

		void NetadapterList::update()
		{
			struct ifaddrs* interfaces;
			struct ifaddrs* interface;
			sa_family_t family;
			char buf[INET6_ADDRSTRLEN];

			Adapters adapterMap;

			if (::getifaddrs(&interfaces) < 0) {
				::syslog(LOG_ERR, "Error calling getifaddrs!");
				return;
			}

			interface = interfaces;

			while (interface != nullptr) {
				if (interface->ifa_addr != nullptr) {
					if (interface->ifa_flags & IFF_MULTICAST) {
							hardwareInfo_t hardwareInfo = getHardwareInfo(interface->ifa_name);
							if (hardwareInfo.isHardware) {
								// If the adapter is not known yet, it will be created.
								unsigned int interfaceIndex = if_nametoindex(interface->ifa_name);
								Netadapter& Adapt = adapterMap[interfaceIndex];

								// An interface may have several ip addresses. Do this only once per interface.
								if(Adapt.m_macAddress.empty()) {
									Adapt.m_index = interfaceIndex;
									Adapt.m_name = interface->ifa_name;
									Adapt.m_macAddress = hardwareInfo.hwAddrString;
									Adapt.m_fwGuid = hardwareInfo.fwGuid;
								}

								family = interface->ifa_addr->sa_family;

								if (family == AF_INET) {
									struct sockaddr_in ipv4_address;
									std::memcpy(&ipv4_address, interface->ifa_addr, sizeof(ipv4_address));

									Ipv4Address addressWithNetmask;
									if (inet_ntop(family, &ipv4_address.sin_addr, buf, sizeof(buf))) {
										addressWithNetmask.address = buf;
									}

									std::memcpy(&ipv4_address, interface->ifa_netmask, sizeof(ipv4_address));
									if (inet_ntop(family, &ipv4_address.sin_addr, buf, sizeof(buf))) {
										addressWithNetmask.netmask = buf;
									}

									// apipa address at the end!
									if (Ipv4Address::isApipaAddress(addressWithNetmask.address)) {
										Adapt.m_ipv4Addresses.push_back(addressWithNetmask);
									} else {
										Adapt.m_ipv4Addresses.push_front(addressWithNetmask);
									}
								} else if (family == AF_INET6) {
									struct sockaddr_in6 ipv6_address;
									std::memcpy(&ipv6_address, interface->ifa_addr, sizeof(ipv6_address));

									Ipv6Address address;
									if (inet_ntop(family, &ipv6_address.sin6_addr, buf, sizeof(buf))) {
										address.address = buf;
									}

									std::memcpy(&ipv6_address, interface->ifa_netmask, sizeof(ipv6_address));
									// calculate prefix: count bits in netmask. ipv6 forces the following form for the prefix 111..11110..00 gaps filled with zero like 101 are not allowed!
									unsigned int bitCount = 0;
									for (unsigned int bytePos = 0; bytePos < sizeof(ipv6_address.sin6_addr); ++bytePos) {
										if (ipv6_address.sin6_addr.s6_addr[bytePos] == 0) {
											break;
										}

										unsigned char byte = ipv6_address.sin6_addr.s6_addr[bytePos];
										unsigned char mask = 1;
										for(unsigned int bitPos=0; bitPos<8; ++bitPos) {
											if(byte & mask) {
												++bitCount;
											}
											mask <<= 1;
										}
									}

									address.prefix = bitCount;
									Adapt.m_ipv6Addresses.push_back(address);
								}
							}
					}
				}

				interface = interface->ifa_next;
			}

			{
				std::lock_guard < std::mutex > lock(m_adaptersMtx);
				m_adapters = adapterMap;
			}

			::freeifaddrs(interfaces);
		}

		NetadapterList::Adapters NetadapterList::get() const
		{
			std::lock_guard < std::mutex > lock(m_adaptersMtx);
			return m_adapters;
		}

		NetadapterList::AdapterArray NetadapterList::getArray() const
		{
			std::lock_guard < std::mutex > lock(m_adaptersMtx);
			AdapterArray result;
			result.reserve(m_adapters.size());

			for (const auto &iter: m_adapters) {
				result.push_back(iter.second);
			}

			return result;
		}

		Netadapter NetadapterList::getAdapterByName(const std::string& adapterName) const
		{
			std::lock_guard < std::mutex > lock(m_adaptersMtx);

			for (const auto &iter: m_adapters) {
				if (iter.second.getName().compare(adapterName) == 0) {
					return iter.second;
				}
			}

			throw hbm::exception::exception("invalid interface");
			// unreachable: return Netadapter();
		}

		Netadapter NetadapterList::getAdapterByInterfaceIndex(unsigned int interfaceIndex) const
		{
			std::lock_guard < std::mutex > lock(m_adaptersMtx);

			Adapters::const_iterator iter = m_adapters.find(interfaceIndex);
			if(iter==m_adapters.end()) {
				throw hbm::exception::exception("invalid interface");
			}

			return iter->second;
		}

		std::string NetadapterList::checkSubnet(const std::string& excludeAdapterName, const communication::Ipv4Address& requestedAddress) const
		{
			std::string requestedSubnet = requestedAddress.getSubnet();

			for (const auto &adapterIter: m_adapters) {
				const communication::Netadapter& adapter = adapterIter.second;
				if (excludeAdapterName != adapter.getName()) {
					communication::AddressesWithNetmask addresses = adapter.getIpv4Addresses();

					for (const communication::Ipv4Address &address: addresses) {
						if (requestedSubnet==address.getSubnet()) {
							return adapter.getName();
						}
					}
				}
			}
			return "";
		}
	}
}


