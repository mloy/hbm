#include <cstdlib>
#include <functional>
#include <iostream>

#include "hbm/sys/eventloop.h"
#include "hbm/communication/netadapterlist.h"
#include "hbm/communication/netlink.h"

static void netlinkCb(hbm::communication::Netlink::event_t event, unsigned int adapterIndex, const std::string& ipv4Address)
{
	switch (event) {
		case hbm::communication::Netlink::NEW:
			// not supported under Windows
			std::cout << "new interface address appeared (adapterindex=" << adapterIndex << ", ipv4 address=" << ipv4Address << ")" << std::endl;
			break;
		case hbm::communication::Netlink::DEL:
			// not supported under Windows
			std::cout << "interface address disappeared (adapterindex=" << adapterIndex << ", ipv4 address=" << ipv4Address << ")" << std::endl;
			break;
		case hbm::communication::Netlink::COMPLETE:
			std::cout << "complete reconfiguration" << std::endl;
			break;
	}
}

int main(int argc, char* argv[])
{
	hbm::sys::EventLoop eventloop;
	hbm::communication::NetadapterList adapters;
	hbm::communication::Netlink netlink(adapters, eventloop);
	
	netlink.start(std::bind(&netlinkCb, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	eventloop.execute();
	return EXIT_SUCCESS;
}