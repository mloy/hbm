// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


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
	static const unsigned int timeToWait = 3500;
	hbm::sys::Timer timer;
	std::chrono::steady_clock::time_point start(std::chrono::steady_clock::now());
	std::chrono::steady_clock::time_point end;
	timer.set(timeToWait);
	ssize_t result = timer.wait();
	end = std::chrono::steady_clock::now();
	std::chrono::milliseconds delta = std::chrono::duration_cast < std::chrono::milliseconds > (end - start);
	unsigned int diff = abs((timeToWait)-delta.count());

	BOOST_CHECK(diff<5);
	BOOST_CHECK(result==1);
}

BOOST_AUTO_TEST_CASE(stop_test)
{
	static const unsigned int timeToWait = 3000;
	hbm::sys::Timer timer;
	std::chrono::steady_clock::time_point start;
	std::chrono::steady_clock::time_point end;

	start = std::chrono::steady_clock::now();
	timer.set(timeToWait);
	timer.cancel();
	// timer is stopped, should return at once
	ssize_t result = timer.wait();

	end = std::chrono::steady_clock::now();
	std::chrono::milliseconds delta = std::chrono::duration_cast < std::chrono::milliseconds > (end - start);
	unsigned int diff = delta.count();
	BOOST_CHECK(diff<5);
	BOOST_CHECK(result==0);
}
