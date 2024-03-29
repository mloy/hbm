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
#include "hbm/sys/executecommand.h"

BOOST_AUTO_TEST_CASE(check_valid_ipaddresses_test)
{
	bool result;
	result = hbm::communication::Ipv4Address::isValidManualAddress("172.19.2.4");
	BOOST_CHECK_EQUAL(result, true);
	result = hbm::communication::Ipv4Address::isValidManualAddress("172.169.254.0");
	BOOST_CHECK_EQUAL(result, true);
}

BOOST_AUTO_TEST_CASE(check_invalid_ipaddresses_test)
{
	bool result;
	result = hbm::communication::Ipv4Address::isValidManualAddress("172.19.2");
	BOOST_CHECK_EQUAL(result, false);
	result = hbm::communication::Ipv4Address::isValidManualAddress("172.19.2.");
	BOOST_CHECK_EQUAL(result, false);
}



BOOST_AUTO_TEST_CASE(check_address_type)
{
	bool result;
	result = hbm::communication::Ipv4Address::isApipaAddress("169.254.2.6");
	BOOST_CHECK_EQUAL(result, true);
	result = hbm::communication::Ipv4Address::isApipaAddress("169.254.2");
	BOOST_CHECK_EQUAL(result, false);
	result = hbm::communication::Ipv4Address::isApipaAddress("169.254.2.");
	BOOST_CHECK_EQUAL(result, false);
	result = hbm::communication::Ipv4Address::isApipaAddress("10.169.254.6");
	BOOST_CHECK_EQUAL(result, false);
	result = hbm::communication::Ipv6Address::isLinkLocalAddress("fe80::40ab:a528:ada6:3da5");
	BOOST_CHECK_EQUAL(result, true);
	result = hbm::communication::Ipv6Address::isLinkLocalAddress("FE80::40ab:a528:ada6:3da5");
	BOOST_CHECK_EQUAL(result, true);
	result = hbm::communication::Ipv6Address::isLinkLocalAddress("fe0::40ab:a528:ada6:3da5");
	BOOST_CHECK_EQUAL(result, false);
}

BOOST_AUTO_TEST_CASE(check_prefix_from_netmask)
{
	int prefix;

	prefix = hbm::communication::Ipv4Address::getPrefixFromNetmask("128.0.0.0");
	BOOST_CHECK_EQUAL(prefix, 1);
	prefix = hbm::communication::Ipv4Address::getPrefixFromNetmask("255.0.0.0");
	BOOST_CHECK_EQUAL(prefix, 8);
	prefix = hbm::communication::Ipv4Address::getPrefixFromNetmask("255.255.0.0");
	BOOST_CHECK_EQUAL(prefix, 16);
	prefix = hbm::communication::Ipv4Address::getPrefixFromNetmask("255.255.255.0");
	BOOST_CHECK_EQUAL(prefix, 24);
	prefix = hbm::communication::Ipv4Address::getPrefixFromNetmask("255.255.255.255");
	BOOST_CHECK_EQUAL(prefix, 32);
	prefix = hbm::communication::Ipv4Address::getPrefixFromNetmask("bla");
	BOOST_CHECK_EQUAL(prefix, -1);
	prefix = hbm::communication::Ipv4Address::getPrefixFromNetmask("255.255.255.");
	BOOST_CHECK_EQUAL(prefix, -1);
	// we do not allow gaps!
	prefix = hbm::communication::Ipv4Address::getPrefixFromNetmask("127.0.0.0");
	BOOST_CHECK_EQUAL(prefix, -1);
	prefix = hbm::communication::Ipv4Address::getPrefixFromNetmask("64.0.0.0");
	BOOST_CHECK_EQUAL(prefix, -1);
}

BOOST_AUTO_TEST_CASE(check_netmask_from_prefix)
{
	std::string netmask;
	netmask = hbm::communication::Ipv4Address::getNetmaskFromPrefix(8);
	BOOST_CHECK_EQUAL(netmask, "255.0.0.0");
	netmask = hbm::communication::Ipv4Address::getNetmaskFromPrefix(16);
	BOOST_CHECK_EQUAL(netmask, "255.255.0.0");
	netmask = hbm::communication::Ipv4Address::getNetmaskFromPrefix(32);
	BOOST_CHECK_EQUAL(netmask, "255.255.255.255");
	netmask = hbm::communication::Ipv4Address::getNetmaskFromPrefix(33);
	BOOST_CHECK_EQUAL(netmask, "");
}

BOOST_AUTO_TEST_CASE(getIpv4MappedIpv6Address)
{
	std::string ipv6Address;
	std::string ipv4Address;

	ipv6Address = "::ffff:192.0.2.128";
	ipv4Address = hbm::communication::Ipv6Address::getIpv4MappedAddress(ipv6Address);
	BOOST_CHECK(ipv4Address.length()>0);

	// wrong prefix
	ipv6Address = "::fff:192.0.2.128";
	ipv4Address = hbm::communication::Ipv6Address::getIpv4MappedAddress(ipv6Address);
	BOOST_CHECK(ipv4Address.length()==0);

	// invalid ipv4 addresses
	ipv6Address = "::ffff:192.0.128";
	ipv4Address = hbm::communication::Ipv6Address::getIpv4MappedAddress(ipv6Address);
	BOOST_CHECK(ipv4Address.length()==0);
	ipv6Address = "::ffff:300.1.0.128";
	ipv4Address = hbm::communication::Ipv6Address::getIpv4MappedAddress(ipv6Address);
	BOOST_CHECK(ipv4Address.length()==0);
	ipv6Address = "::ffff:192.0.2.";
	ipv4Address = hbm::communication::Ipv6Address::getIpv4MappedAddress(ipv6Address);
	BOOST_CHECK(ipv4Address.length()==0);

}

BOOST_AUTO_TEST_CASE(check_forbidden_ipaddresses_test)
{
	bool result;
	result = hbm::communication::Ipv4Address::isValidManualAddress("not an address");
	BOOST_CHECK_EQUAL(result, false);
	result = hbm::communication::Ipv4Address::isValidManualAddress("127.0.0.1"); // loopback
	BOOST_CHECK_EQUAL(result, false);
	result = hbm::communication::Ipv4Address::isValidManualAddress("169.254.0.1"); // APIPA
	BOOST_CHECK_EQUAL(result, false);
	result = hbm::communication::Ipv4Address::isValidManualAddress("224.4.7.1"); // multicast
	BOOST_CHECK_EQUAL(result, false);
	result = hbm::communication::Ipv4Address::isValidManualAddress("254.4.7.1"); // experimental
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


	hbm::communication::NetadapterList::AdapterArray adapterArray = netadapterList.getArray();
	BOOST_CHECK_EQUAL(adapters.begin()->second.getIndex(), adapterArray[0].getIndex());

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

// Test does not work withing docker image. Command "ip" not available
//BOOST_AUTO_TEST_CASE(check_ipv4_gateway)
//{
//	static const std::string cmd = "ip route | grep default";
//	std::string gateway = hbm::communication::Netadapter::getIpv4DefaultGateway();

//	std::string result = hbm::sys::executeCommand(cmd);
//	hbm::string::tokens tokens = hbm::string::split(result, ' ');
//	if (gateway.empty() && tokens.empty()) {
//		// there is no default gateway!
//	} else {
//		BOOST_CHECK(tokens.size()>=3);
//		if (tokens.size()>=3) {
//			std::string gateway2 = tokens[2];
//			BOOST_CHECK_EQUAL(gateway, gateway2);
//		}
//	}
//}
