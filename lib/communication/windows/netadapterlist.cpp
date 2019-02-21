// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#include <iomanip>
#include <sstream>
#include <string>
#include <stdint.h>
#include <iterator>
#include <mutex>
#include <cstring>

#include <windows.h>
#include <iphlpapi.h>
#define syslog fprintf
#define LOG_ERR stderr

#include "hbm/exception/exception.hpp"

#include "hbm/communication/netadapterlist.h"
#include "hbm/communication/netadapter.h"

namespace hbm {
	namespace communication {
		NetadapterList::NetadapterList()
		{
			update();
		}

		void NetadapterList::update()
		{
			IP_ADAPTER_INFO* pAdptInfo = NULL;
			IP_ADAPTER_INFO* pNextAd = NULL;
			ULONG ulLen = 0;
			DWORD erradapt;

			//This call returns the number of network adapter in ulLen
			erradapt = ::GetAdaptersInfo(pAdptInfo, &ulLen);

			if (erradapt == ERROR_BUFFER_OVERFLOW) {
				pAdptInfo = reinterpret_cast < IP_ADAPTER_INFO* >(new UINT8[ulLen]);
				erradapt = ::GetAdaptersInfo(pAdptInfo, &ulLen);
			}

			if (erradapt == ERROR_SUCCESS) {
				std::lock_guard < std::mutex > lock(m_adaptersMtx);
				m_adapters.clear();
				// initialize the pointer we use the move through
				// the list.
				pNextAd = pAdptInfo;

				// loop through for all available interfaces and setup an associated
				// CNetworkAdapter class.
				while (pNextAd) {
					std::stringstream macStream;
					Netadapter Adapt;
					std::vector < std::string > GatewayList;
					IP_ADDR_STRING* pNext	= NULL;

					unsigned int adapterIndex = pNextAd->Index;
					Adapt.m_index = adapterIndex;
					Adapt.m_macAddress.clear();

					for (unsigned int i = 0; i < pNextAd->AddressLength; i++) {
						if (i > 0) {
							macStream << ":";
						}

						macStream << std::hex << std::setw(2) << std::setfill('0') << static_cast < unsigned int >(pNextAd->Address[i]) << std::dec;
					}

					Adapt.m_macAddress = macStream.str();

					Ipv4Address addressWithNetmask;

					if (pNextAd->CurrentIpAddress) {
						addressWithNetmask.address = pNextAd->CurrentIpAddress->IpAddress.String;
						addressWithNetmask.netmask = pNextAd->CurrentIpAddress->IpMask.String;
					} else {
						addressWithNetmask.address = pNextAd->IpAddressList.IpAddress.String;
						addressWithNetmask.netmask = pNextAd->IpAddressList.IpMask.String;
					}

					// there might be several addresses per interface
					if (addressWithNetmask.address != "0.0.0.0") {
						// 0.0.0.0 does mean "no address"
						Adapt.m_ipv4Addresses.push_back(addressWithNetmask);
					}

					// an adapter usually has just one gateway however the provision exists
					// for more than one so to "play" as nice as possible we allow for it here
					// as well.
					pNext = &(pNextAd->GatewayList);

					while (pNext) {
						GatewayList.push_back(pNext->IpAddress.String);
						pNext = pNext->Next;
					}


					Adapt.m_name = pNextAd->Description;
					if ((Adapt.getIpv4Addresses().empty() == false) || (Adapt.getIpv6Addresses().empty() == false)) {
						// T-C software does not want interfaces without address!
						m_adapters[adapterIndex] = Adapt;
					}

					// move forward to the next adapter in the list so
					// that we can collect its information.
					pNextAd = pNextAd->Next;
				}
			}

			// free any memory we allocated from the heap before exit.
			delete[] pAdptInfo;
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

			for(Adapters::const_iterator iter = m_adapters.begin(); iter!=m_adapters.end(); ++iter) {
				result.push_back(iter->second);
			}

			return result;
		}

		Netadapter NetadapterList::getAdapterByName(const std::string& adapterName) const
		{
			std::lock_guard < std::mutex > lock(m_adaptersMtx);

			for (Adapters::const_iterator iter = m_adapters.begin(); iter != m_adapters.end(); ++iter) {
				if (iter->second.getName().compare(adapterName) == 0) {
					return iter->second;
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

                        for (communication::NetadapterList::Adapters::const_iterator adapterIter=m_adapters.begin(); adapterIter!=m_adapters.end(); ++adapterIter ) {
                                const communication::Netadapter& adapter = adapterIter->second;
                                if (excludeAdapterName != adapter.getName()) {
                                        communication::AddressesWithNetmask addresses = adapter.getIpv4Addresses();

                                        for (communication::AddressesWithNetmask::const_iterator addressIter = addresses.begin(); addressIter!=addresses.end(); ++addressIter) {
                                                const communication::Ipv4Address& address = *addressIter;
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


