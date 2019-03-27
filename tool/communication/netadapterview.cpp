#include <iostream>

#include "hbm/communication/netadapterlist.h"
#include "hbm/communication/netlink.h"


int main()
{
	std::cout << "netadapterview" << std::endl;
	hbm::communication::NetadapterList adapterList;
	hbm::communication::NetadapterList::Adapters adapters = adapterList.get();

	for (const auto iter: adapters) {
		const hbm::communication::Netadapter& adapter = iter.second;
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
	return EXIT_SUCCESS;
}
