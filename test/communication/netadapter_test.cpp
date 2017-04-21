// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#ifndef _WIN32
#define BOOST_TEST_DYN_LINK
#endif
#define BOOST_TEST_MAIN

#define BOOST_TEST_MODULE netadapter tests

#include <boost/test/unit_test.hpp>

#include "hbm/communication/netadapter.h"
#include "hbm/communication/netadapterlist.h"

#include "hbm/string/split.h"

BOOST_AUTO_TEST_CASE(check_valid_ipaddresses_test)
{
	bool result;
	result = hbm::communication::Netadapter::isValidManualIpv4Address("172.19.2.4");
	BOOST_CHECK_EQUAL(result, true);
	result = hbm::communication::Netadapter::isValidManualIpv4Address("172.169.254.0");
	BOOST_CHECK_EQUAL(result, true);
}

BOOST_AUTO_TEST_CASE(check_invalid_ipaddresses_test)
{
	bool result;
	result = hbm::communication::Netadapter::isValidManualIpv4Address("172.19.2");
	BOOST_CHECK_EQUAL(result, false);
}



BOOST_AUTO_TEST_CASE(check_address_type)
{
	bool result;
	result = hbm::communication::Netadapter::isApipaAddress("169.254.2.6");
	BOOST_CHECK_EQUAL(result, true);
	result = hbm::communication::Netadapter::isApipaAddress("169.254.2");
	BOOST_CHECK_EQUAL(result, false);
	result = hbm::communication::Netadapter::isApipaAddress("10.169.254.6");
	BOOST_CHECK_EQUAL(result, false);
	result = hbm::communication::Netadapter::isIpv6LinkLocalAddress("fe80::40ab:a528:ada6:3da5");
	BOOST_CHECK_EQUAL(result, true);
	result = hbm::communication::Netadapter::isIpv6LinkLocalAddress("fe0::40ab:a528:ada6:3da5");
	BOOST_CHECK_EQUAL(result, false);
}

BOOST_AUTO_TEST_CASE(check_prefix_from_netmask)
{
	int prefix;
	prefix = hbm::communication::Netadapter::getPrefixFromIpv4Netmask("255.0.0.0");
	BOOST_CHECK_EQUAL(prefix, 8);
	prefix = hbm::communication::Netadapter::getPrefixFromIpv4Netmask("255.255.0.0");
	BOOST_CHECK_EQUAL(prefix, 16);
	prefix = hbm::communication::Netadapter::getPrefixFromIpv4Netmask("255.255.255.0");
	BOOST_CHECK_EQUAL(prefix, 24);
	prefix = hbm::communication::Netadapter::getPrefixFromIpv4Netmask("255.255.255.255");
	BOOST_CHECK_EQUAL(prefix, 32);
	prefix = hbm::communication::Netadapter::getPrefixFromIpv4Netmask("bla");
	BOOST_CHECK_EQUAL(prefix, -1);
}

BOOST_AUTO_TEST_CASE(check_netmask_from_prefix)
{
	std::string netmask;
	netmask = hbm::communication::Netadapter::getIpv4NetmaskFromPrefix(8);
	BOOST_CHECK_EQUAL(netmask, "255.0.0.0");
	netmask = hbm::communication::Netadapter::getIpv4NetmaskFromPrefix(16);
	BOOST_CHECK_EQUAL(netmask, "255.255.0.0");
	netmask = hbm::communication::Netadapter::getIpv4NetmaskFromPrefix(32);
	BOOST_CHECK_EQUAL(netmask, "255.255.255.255");
	netmask = hbm::communication::Netadapter::getIpv4NetmaskFromPrefix(33);
	BOOST_CHECK_EQUAL(netmask, "");
}

BOOST_AUTO_TEST_CASE(check_forbidden_ipaddresses_test)
{
	bool result;
	result = hbm::communication::Netadapter::isValidManualIpv4Address("not an address");
	BOOST_CHECK_EQUAL(result, false);
	result = hbm::communication::Netadapter::isValidManualIpv4Address("0.0.0.0");
	BOOST_CHECK_EQUAL(result, false);
	result = hbm::communication::Netadapter::isValidManualIpv4Address("127.0.0.1"); // loopback
	BOOST_CHECK_EQUAL(result, false);
	result = hbm::communication::Netadapter::isValidManualIpv4Address("169.254.0.1"); // APIPA
	BOOST_CHECK_EQUAL(result, false);
	result = hbm::communication::Netadapter::isValidManualIpv4Address("224.4.7.1"); // multicast
	BOOST_CHECK_EQUAL(result, false);
	result = hbm::communication::Netadapter::isValidManualIpv4Address("254.4.7.1"); // experimental
	BOOST_CHECK_EQUAL(result, false);
}

BOOST_AUTO_TEST_CASE(check_mac_address)
{
	hbm::communication::NetadapterList::Adapters adapters = hbm::communication::NetadapterList().get();
	if (adapters.empty()) {
		return;
	}
	
	std::string macAddress = adapters.begin()->second.getMacAddressString();
	BOOST_CHECK_GT(macAddress.length(), 0);
	hbm::string::tokens tokens = hbm::string::split(macAddress, ':');
	BOOST_CHECK_EQUAL(tokens.size(), 6);
	
}

BOOST_AUTO_TEST_CASE(check_subnet)
{
	hbm::communication::Ipv4Address address;
	std::string subnet;
	address.address = "172.19.1.2";
	address.netmask = "255.255.0.0";
	subnet = address.getSubnet();
	BOOST_CHECK_EQUAL(subnet, "172.19.0.0");

	address.address = "172.130.1.2";
	address.netmask = "255.128.0.0";
	subnet = address.getSubnet();
	BOOST_CHECK_EQUAL(subnet, "172.128.0.0");

	address.address = "172.130.1.2";
	address.netmask = "255.128.0.2";
	subnet = address.getSubnet();
	BOOST_CHECK_EQUAL(subnet, "172.128.0.2");
}

BOOST_AUTO_TEST_CASE(check_occupied_subnet)
{
	// get first ipv4 address of first interface to provoke conflict.
	std::string occupyingInterfaceName;
	std::string interfaceName;
	hbm::communication::NetadapterList netadapterList;
	hbm::communication::AddressesWithNetmask addresses;

	// get the first active adapter.
	hbm::communication::Netadapter adapter;
	hbm::communication::NetadapterList::Adapters adapters = netadapterList.get();
	for( hbm::communication::NetadapterList::Adapters::const_iterator iter=adapters.begin(); iter!=adapters.end(); ++iter) {
		adapter = iter->second;
		addresses = adapter.getIpv4Addresses();
		if (addresses.empty()==false) {
			break;
		}
	}

	if (addresses.empty()) {
		BOOST_TEST_MESSAGE("No interface with ipv4 address available. Test can not be performed!");
		return;
	}

	hbm::communication::Ipv4Address firstAddress = *addresses.begin();
	interfaceName = adapter.getName();
	
	occupyingInterfaceName = netadapterList.checkSubnet("", firstAddress);
	BOOST_CHECK_EQUAL(occupyingInterfaceName, interfaceName);


	// check with exclusion of this interface
	occupyingInterfaceName = netadapterList.checkSubnet(interfaceName, firstAddress);
	BOOST_CHECK_EQUAL(occupyingInterfaceName, "");

	
	// localhost is not contained in netadapterlist, hence nobody does occupy "127.0.0.1"
	hbm::communication::Ipv4Address localHostAddress;
	localHostAddress.address = "127.0.0.1";
	localHostAddress.netmask = "255.0.0.0";
	
	occupyingInterfaceName = netadapterList.checkSubnet(interfaceName, localHostAddress);
	BOOST_CHECK_EQUAL(occupyingInterfaceName, "");

}
