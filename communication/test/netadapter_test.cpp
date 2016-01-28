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

BOOST_AUTO_TEST_CASE(check_ipv6)
{
	hbm::communication::NetadapterList adapterlist;
	hbm::communication::NetadapterList::tAdapters adapters = adapterlist.get();
	if (adapters.empty()) {
		return;
	}
	
	hbm::communication::Netadapter adapter = adapters.begin()->second;
	BOOST_CHECK_GT(adapter.getIpv6Addresses().size(), 0);
}

BOOST_AUTO_TEST_CASE(check_valid_ipaddresses_test)
{
    bool result;
    result = hbm::communication::Netadapter::isValidManualIpV4Address("172.19.2.4");
    BOOST_CHECK_EQUAL(result, true);
    result = hbm::communication::Netadapter::isValidManualIpV4Address("172.169.254.0");
    BOOST_CHECK_EQUAL(result, true);
}

BOOST_AUTO_TEST_CASE(check_prefix_from_netmask)
{
    int result;
    result = hbm::communication::Netadapter::getPrefixFromNetmask("255.255.0.0");
    BOOST_CHECK_EQUAL(result, 16);
    result = hbm::communication::Netadapter::getPrefixFromNetmask("bla");
    BOOST_CHECK_EQUAL(result, -1);
}



BOOST_AUTO_TEST_CASE(check_forbidden_ipaddresses_test)
{
    bool result;
    result = hbm::communication::Netadapter::isValidManualIpV4Address("not an address");
    BOOST_CHECK_EQUAL(result, false);
    result = hbm::communication::Netadapter::isValidManualIpV4Address("0.0.0.0");
    BOOST_CHECK_EQUAL(result, false);
    result = hbm::communication::Netadapter::isValidManualIpV4Address("127.0.0.1"); // loopback
    BOOST_CHECK_EQUAL(result, false);
    result = hbm::communication::Netadapter::isValidManualIpV4Address("169.254.0.1"); // APIPA
    BOOST_CHECK_EQUAL(result, false);
    result = hbm::communication::Netadapter::isValidManualIpV4Address("224.4.7.1"); // multicast
    BOOST_CHECK_EQUAL(result, false);
    result = hbm::communication::Netadapter::isValidManualIpV4Address("254.4.7.1"); // experimental
    BOOST_CHECK_EQUAL(result, false);
}
