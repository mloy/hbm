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
			enumAdapters();
		}

		void NetadapterList::enumAdapters()
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

					ipv4Address_t addressWithNetmask;

					if (pNextAd->CurrentIpAddress) {
						addressWithNetmask.address = pNextAd->CurrentIpAddress->IpAddress.String;
						addressWithNetmask.netmask = pNextAd->CurrentIpAddress->IpMask.String;
					} else {
						addressWithNetmask.address = pNextAd->IpAddressList.IpAddress.String;
						addressWithNetmask.netmask = pNextAd->IpAddressList.IpMask.String;
					}

					// there might be several addresses per interface
					Adapt.m_ipv4Addresses.push_back(addressWithNetmask);

					// an adapter usually has just one gateway however the provision exists
					// for more than one so to "play" as nice as possible we allow for it here
					// as well.
					pNext = &(pNextAd->GatewayList);

					while (pNext) {
						GatewayList.push_back(pNext->IpAddress.String);
						pNext = pNext->Next;
					}


					if (addressWithNetmask.address != "0.0.0.0") { // HBM only wants connected Interfaces to be enumerated

						Adapt.m_name = pNextAd->Description;
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


		NetadapterList::tAdapters NetadapterList::get() const
		{
			std::lock_guard < std::mutex > lock(m_adaptersMtx);
			return m_adapters;
		}

		NetadapterList::tAdapterArray NetadapterList::getArray() const
		{
			std::lock_guard < std::mutex > lock(m_adaptersMtx);
			tAdapterArray result;
			result.reserve(m_adapters.size());

			for(tAdapters::const_iterator iter = m_adapters.begin(); iter!=m_adapters.end(); ++iter) {
				result.push_back(iter->second);
			}

			return result;
		}

		Netadapter NetadapterList::getAdapterByName(const std::string& adapterName) const
		{
			std::lock_guard < std::mutex > lock(m_adaptersMtx);

			for (tAdapters::const_iterator iter = m_adapters.begin(); iter != m_adapters.end(); ++iter) {
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

			tAdapters::const_iterator iter = m_adapters.find(interfaceIndex);
			if(iter==m_adapters.end()) {
				throw hbm::exception::exception("invalid interface");
			}

			return iter->second;
		}

		void NetadapterList::update()
		{
			enumAdapters();
		}
	}
}


