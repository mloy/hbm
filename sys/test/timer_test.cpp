/*
 * Copyright (C) 2007 Hottinger Baldwin Messtechnik GmbH
 * Im Tiefen See 45
 * 64293 Darmstadt
 * Germany
 * http://www.hbm.com
 * All rights reserved
 *
 * The copyright to the computer program(s) herein is the property of
 * Hottinger Baldwin Messtechnik GmbH (HBM), Germany. The program(s)
 * may be used and/or copied only with the written permission of HBM
 * or in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 * This copyright notice must not be removed.
 *
 * This Software is licenced by the
 * "General supply and license conditions for software"
 * which is part of the standard terms and conditions of sale from HBM.
*/

/** @file */


#ifndef _WIN32
#define BOOST_TEST_DYN_LINK
#endif
#define BOOST_TEST_MAIN

#define BOOST_TEST_MODULE executecommand tests

#include <stdint.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <boost/test/unit_test.hpp>

#include "hbm/sys/timer.h"
#include "hbm/exception/exception.hpp"


BOOST_AUTO_TEST_CASE(notstarted_test)
{
	hbm::sys::Timer timer;
	ssize_t result = timer.wait();
	BOOST_CHECK(result==-1);
}


BOOST_AUTO_TEST_CASE(wait_test)
{
	static const unsigned int timeToWait = 3;
	hbm::sys::Timer timer;
	std::chrono::time_point<std::chrono::steady_clock> start(std::chrono::steady_clock::now());
	std::chrono::time_point<std::chrono::steady_clock> end;
	timer.set(3);
	ssize_t result = timer.wait();
	end = std::chrono::steady_clock::now();
	std::chrono::milliseconds delta = std::chrono::duration_cast < std::chrono::milliseconds > (end - start);
	unsigned int diff = abs((timeToWait*1000)-delta.count());

	BOOST_CHECK(diff<5);
	BOOST_CHECK(result==1);
}

BOOST_AUTO_TEST_CASE(stop_test)
{
	hbm::sys::Timer timer;
	std::chrono::time_point<std::chrono::steady_clock> start;
	std::chrono::time_point<std::chrono::steady_clock> end;

	start = std::chrono::steady_clock::now();
	timer.set(3);
	timer.cancel();
	// timer is stopped, should return at once
	ssize_t result = timer.wait();

	end = std::chrono::steady_clock::now();
	std::chrono::milliseconds delta = std::chrono::duration_cast < std::chrono::milliseconds > (end - start);
	unsigned int diff = delta.count();
	BOOST_CHECK(diff<5);
	BOOST_CHECK(result==0);
}
