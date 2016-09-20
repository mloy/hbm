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
//		case hbm::communication::Netlink::NEW:
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
//		case hbm::communication::Netlink::DEL:
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
	hbm::communication::NetadapterList::tAdapters adapters = adapterList.get();

	for (hbm::communication::NetadapterList::tAdapters::const_iterator iter=adapters.begin(); iter!=adapters.end(); ++iter) {
		const hbm::communication::Netadapter& adapter = iter->second;
		std::cout << adapter.getName() << std::endl;
		hbm::communication::addressesWithNetmask_t ipv4addresses = adapter.getIpv4Addresses();
		for (hbm::communication::addressesWithNetmask_t::const_iterator addressIter = ipv4addresses.begin(); addressIter!=ipv4addresses.end(); ++addressIter) {
			const hbm::communication::ipv4Address_t& ipV4Address = *addressIter;
			std::cout << "\t" << ipV4Address.address << " " << ipV4Address.netmask << std::endl;
		}

		hbm::communication::addressesWithPrefix_t ipv6addresses = adapter.getIpv6Addresses();
		for (hbm::communication::addressesWithPrefix_t::const_iterator addressIter = ipv6addresses.begin(); addressIter!=ipv6addresses.end(); ++addressIter) {
			const hbm::communication::ipv6Address_t& ipV6Address = *addressIter;
			std::cout << "\t" << ipV6Address.address << "/" << ipV6Address.prefix << std::endl;
		}


	}


//	hbm::sys::EventLoop eventloop;
//	hbm::communication::Netlink netlink(adapters, eventloop);
	
//	netlink.start(std::bind(&netlinkCb, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
//	eventloop.execute();
//	return EXIT_SUCCESS;
}
