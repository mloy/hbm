//#include <cstdlib>
//#include <functional>
#include <iostream>

//#include "hbm/sys/eventloop.h"
#include "hbm/communication/netadapterlist.h"
#include "hbm/communication/netlink.h"

//static hbm::communication::NetadapterList adapters;

//static void netlinkCb(hbm::communication::Netlink::event_t event, unsigned int adapterIndex, const std::string& ipv4Address)
//{
//	static unsigned int eventCount = 0;

//	++eventCount;
//	std::cout << eventCount << ": ";
//	switch (event) {
//		case hbm::communication::Netlink::ADDRESS_ADDED:
//			{
//				// not supported under Windows
//				std::string adapterName;
//				try {
//					adapterName = adapters.getAdapterByInterfaceIndex(adapterIndex).getName();
//				} catch(const std::runtime_error& e) {
//				}
			
//				std::cout << "new interface address appeared (adapter='" << adapterName << "', ipv4 address=" << ipv4Address << ")" << std::endl;
//			}
//			break;
//		case hbm::communication::Netlink::ADDRESSE_REMOVED:
//			{
//				// not supported under Windows
//				std::string adapterName;
//				try {
//					adapterName = adapters.getAdapterByInterfaceIndex(adapterIndex).getName();
//				} catch(const std::runtime_error& e) {
//				}
			
//				std::cout << "interface address disappeared (adapter'=" << adapterName << "', ipv4 address=" << ipv4Address << ")" << std::endl;
//			}
//			break;
//		case hbm::communication::Netlink::COMPLETE:
//			std::cout << "complete reconfiguration" << std::endl;
//			break;
//	}
//}

int main()
{
	std::cout << "netadapterview" << std::endl;
	hbm::communication::NetadapterList adapterList;
	hbm::communication::NetadapterList::Adapters adapters = adapterList.get();

	for (hbm::communication::NetadapterList::Adapters::const_iterator iter=adapters.begin(); iter!=adapters.end(); ++iter) {
		const hbm::communication::Netadapter& adapter = iter->second;
		std::cout << adapter.getName() << std::endl;
		hbm::communication::AddressesWithNetmask ipv4addresses = adapter.getIpv4Addresses();
		for (hbm::communication::AddressesWithNetmask::const_iterator addressIter = ipv4addresses.begin(); addressIter!=ipv4addresses.end(); ++addressIter) {
			const hbm::communication::Ipv4Address& ipV4Address = *addressIter;
			std::cout << "\t" << ipV4Address.address << " " << ipV4Address.netmask << std::endl;
		}

		hbm::communication::AddressesWithPrefix ipv6addresses = adapter.getIpv6Addresses();
		for (hbm::communication::AddressesWithPrefix::const_iterator addressIter = ipv6addresses.begin(); addressIter!=ipv6addresses.end(); ++addressIter) {
			const hbm::communication::Ipv6Address& ipV6Address = *addressIter;
			std::cout << "\t" << ipV6Address.address << "/" << ipV6Address.prefix << std::endl;
		}


	}


//	hbm::sys::EventLoop eventloop;
//	hbm::communication::Netlink netlink(adapters, eventloop);
	
//	netlink.start(std::bind(&netlinkCb, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
//	eventloop.execute();
//	return EXIT_SUCCESS;
}
