#include <cstdlib>
#include <functional>
#include <iostream>

#include "hbm/sys/eventloop.h"
#include "hbm/communication/netadapterlist.h"
#include "hbm/communication/netlink.h"

static hbm::communication::NetadapterList adapters;

static void netlinkCb(hbm::communication::Netlink::event_t event, unsigned int adapterIndex, const std::string& ipv4Address)
{
	static unsigned int eventCount = 0;
	std::string adapterName;
	try {
		adapterName = adapters.getAdapterByInterfaceIndex(adapterIndex).getName();
	} catch(const std::runtime_error&) {
		// to be ignored because adapter does not exist anymore
	}

	++eventCount;
	std::cout << eventCount << ": ";
	switch (event) {
		case hbm::communication::Netlink::ADDRESS_ADDED:
			// not supported under Windows
			std::cout << "new interface address appeared (adapter " << adapterIndex << " (" << adapterName << "), ipv4 address=" << ipv4Address << ")" << std::endl;
			break;
		case hbm::communication::Netlink::ADDRESS_REMOVED:
			// not supported under Windows
			std::cout << "interface address disappeared (adapter " << adapterIndex << " (" << adapterName << "), ipv4 address=" << ipv4Address << ")" << std::endl;
			break;
		case hbm::communication::Netlink::COMPLETE:
			std::cout << "complete reconfiguration" << std::endl;
			break;

		case hbm::communication::Netlink::LINK_ADDED:
			// not supported under Windows
			std::cout << "interface " << adapterIndex << " (" << adapterName << ") got up" << std::endl;
			break;

		case hbm::communication::Netlink::LINK_REMOVED:
			// not supported under Windows
			std::cout << "interface " << adapterIndex << " (" << adapterName << ") went down" << std::endl;
			break;
	}
}

int main()
{
	hbm::sys::EventLoop eventloop;
	hbm::communication::Netlink netlink(adapters, eventloop);
	
	netlink.start(std::bind(&netlinkCb, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	eventloop.execute();
	return EXIT_SUCCESS;
}
