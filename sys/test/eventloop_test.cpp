// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#ifndef _WIN32
#define BOOST_TEST_DYN_LINK
#endif
#define BOOST_TEST_MAIN

#define BOOST_TEST_MODULE eventloop tests
#include <chrono>

#include <boost/test/unit_test.hpp>

#include "hbm/sys/eventloop.h"
#include "hbm/sys/timer.h"
#include "hbm/sys/notifier.h"
#include "hbm/exception/exception.hpp"




/// by returning error, the execute() method, that is doing the eventloop, exits
static ssize_t eventHandlerStop()
{
	return -1;
}

static unsigned int eventsLeft;

/// by returning error, the execute() method, that is doing the eventloop, exits
static ssize_t eventHandlerStopAfterN()
{
	--eventsLeft;
	if (eventsLeft==0) {
		return -1;
	} else {
		return 0;
	}
}


BOOST_AUTO_TEST_CASE(norify_test)
{
	int result;
	hbm::sys::EventLoop eventLoop;
	hbm::sys::Notifier notifier;
	static const std::chrono::milliseconds duration(100);

	// wait time out to happen
	eventLoop.addEvent(notifier.getFd(), &eventHandlerStop);
	result = eventLoop.execute_for(duration);
	BOOST_CHECK(result==0);

	// callback of notifier signals error
	notifier.notify();
	result = eventLoop.execute_for(duration);
	BOOST_CHECK(result==-1);

}

BOOST_AUTO_TEST_CASE(waitforend_test)
{
	hbm::sys::EventLoop eventLoop;

	static const std::chrono::milliseconds duration(100);

	std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	int result = eventLoop.execute_for(duration);
	std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();

	std::chrono::milliseconds delta = std::chrono::duration_cast < std::chrono::milliseconds > (endTime-startTime-duration);

	BOOST_CHECK(result==0);
	BOOST_CHECK(delta.count()<5);
}

BOOST_AUTO_TEST_CASE(timerevent_test)
{
	static const unsigned int timerCycle = 100;
	static const unsigned int timerCount = 10;
	static const std::chrono::milliseconds duration(timerCycle*3);
	hbm::sys::EventLoop eventLoop;

	hbm::sys::Timer timer(timerCycle);
	eventLoop.addEvent(timer.getFd(), &eventHandlerStop);

	int result = eventLoop.execute_for(duration);
	BOOST_CHECK(result==-1);
}

BOOST_AUTO_TEST_CASE(severaltimerevents_test)
{
	static const unsigned int timerCycle = 100;
	static const unsigned int timerCount = 10;
	static const std::chrono::milliseconds duration(timerCycle*3);
	hbm::sys::EventLoop eventLoop;

	eventsLeft = 2;

	hbm::sys::Timer timer(timerCycle);
	eventLoop.addEvent(timer.getFd(), &eventHandlerStop);

	int result = eventLoop.execute_for(duration);
	BOOST_CHECK(result==-1);
}
