// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#ifndef _WIN32
#define BOOST_TEST_DYN_LINK
#endif
#define BOOST_TEST_MAIN

#define BOOST_TEST_MODULE eventloop tests
#include <iostream>
#include <chrono>
#include <thread>
#include <functional>
#include <vector>

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

static ssize_t eventHandlerPrint()
{
	std::cout << __FUNCTION__ << std::endl;
	return 0;
}



/// by returning error, the execute() method, that is doing the eventloop, exits
static ssize_t eventHandlerStopAfterN(unsigned int* pEventsLeft)
{
	--(*pEventsLeft);
	if ((*pEventsLeft)==0) {
		return -1;
	} else {
		return 0;
	}
}

/// by returning error, the execute() method, that is doing the eventloop, exits
static ssize_t eventHandlerIncrement(unsigned int* pValue)
{
	++(*pValue);
	return 0;
}



/// start the eventloop in a separate thread wait some time and stop it.
BOOST_AUTO_TEST_CASE(stop_test)
{
	hbm::sys::EventLoop eventLoop;

	static const std::chrono::milliseconds waitDuration(300);

	std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	std::thread worker(std::bind(&hbm::sys::EventLoop::execute, &eventLoop));
	std::this_thread::sleep_for(waitDuration);
	eventLoop.stop();
	worker.join();
	std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();

	std::chrono::milliseconds delta = std::chrono::duration_cast < std::chrono::milliseconds > (endTime-startTime);

	BOOST_CHECK_MESSAGE(delta.count()>=300, "unexpected wait difference of " << std::to_string(delta.count()) << "ms");
}


BOOST_AUTO_TEST_CASE(waitforend_test)
{
	hbm::sys::EventLoop eventLoop;

	static const std::chrono::milliseconds duration(100);

	std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	int result = eventLoop.execute_for(duration);
	std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();

	std::chrono::milliseconds delta = std::chrono::duration_cast < std::chrono::milliseconds > (endTime-startTime);

	BOOST_CHECK_EQUAL(result, 0);
	BOOST_CHECK_GE(delta.count(), duration.count()-3);
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

BOOST_AUTO_TEST_CASE(timerevent_test)
{
	static const unsigned int timerCycle = 100;
	static const std::chrono::milliseconds duration(timerCycle*3);
	hbm::sys::EventLoop eventLoop;

	hbm::sys::Timer timer(timerCycle, true);

	eventLoop.addEvent(timer.getFd(), &eventHandlerStop);
	int result = eventLoop.execute_for(duration);

	BOOST_CHECK_EQUAL(result, -1);
}

/// add timer while event loop is already running.
BOOST_AUTO_TEST_CASE(addevent_wile_executing_test)
{
	static const unsigned int timerCycle = 100;
	static const std::chrono::milliseconds duration(timerCycle*3);
	hbm::sys::EventLoop eventLoop;

	hbm::sys::Timer timer(timerCycle, true);

	std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	std::thread worker(std::bind(&hbm::sys::EventLoop::execute_for, &eventLoop, duration));
	eventLoop.addEvent(timer.getFd(), &eventHandlerStop);

	worker.join(); // should return when timer expires
	std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();

	std::chrono::milliseconds delta = std::chrono::duration_cast < std::chrono::milliseconds > (endTime-startTime);

	BOOST_CHECK_LT(delta.count(), duration.count());
}

/// remove timer while event loop is already running.
BOOST_AUTO_TEST_CASE(eraseevent_wile_executing_test)
{
	static const unsigned int timerCycle = 100;
	static const std::chrono::milliseconds duration(timerCycle*3);
	hbm::sys::EventLoop eventLoop;

	hbm::sys::Timer timer(timerCycle, true);

	std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	eventLoop.addEvent(timer.getFd(), &eventHandlerStop);

	std::thread worker(std::bind(&hbm::sys::EventLoop::execute_for, &eventLoop, duration));

	eventLoop.eraseEvent(timer.getFd());

	worker.join(); // should return on timeout of event loop
	std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();

	std::chrono::milliseconds delta = std::chrono::duration_cast < std::chrono::milliseconds > (endTime-startTime);

	BOOST_CHECK_GT(delta.count(), duration.count());
}

/// cancel timer while event loop is already running.
BOOST_AUTO_TEST_CASE(cancelTimer_wile_executing_test)
{
	static const unsigned int timerCycle = 100;
	static const std::chrono::milliseconds duration(timerCycle*3);
	hbm::sys::EventLoop eventLoop;

	hbm::sys::Timer timer(timerCycle, true);

	std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	eventLoop.addEvent(timer.getFd(), &eventHandlerStop);

	std::thread worker(std::bind(&hbm::sys::EventLoop::execute_for, &eventLoop, duration));

	timer.cancel();

	worker.join(); // should return on timeout of event loop
	std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();

	std::chrono::milliseconds delta = std::chrono::duration_cast < std::chrono::milliseconds > (endTime-startTime);

	BOOST_CHECK_GT(delta.count(), duration.count());
}



BOOST_AUTO_TEST_CASE(severaltimerevents_test)
{
	static const unsigned int timerCycle = 10;
	static const unsigned int timerCount = 10;
	static const std::chrono::milliseconds duration(timerCycle * timerCount);
	hbm::sys::EventLoop eventLoop;

	unsigned int eventsLeft = 2;

	hbm::sys::Timer timer(timerCycle, true);
	eventLoop.addEvent(timer.getFd(), std::bind(&eventHandlerStopAfterN, &eventsLeft));

	int result = eventLoop.execute_for(duration);
	BOOST_CHECK_EQUAL(eventsLeft, 0);
	BOOST_CHECK(result == -1);
}

BOOST_AUTO_TEST_CASE(severaltimers_test)
{
	static const unsigned int timerCycle = 10;
	static const unsigned int timerCount = 10;
	static const std::chrono::milliseconds duration(timerCycle * timerCount);
	hbm::sys::EventLoop eventLoop;

	unsigned int counter = 0;

	typedef std::vector < hbm::sys::Timer > timers_t;
	timers_t timers(10);

	for (timers_t::iterator iter = timers.begin(); iter != timers.end(); ++iter) {
		hbm::sys::Timer& timer = *iter;
		timer.set(timerCycle, false);
		eventLoop.addEvent(timer.getFd(), std::bind(&eventHandlerIncrement, &counter));
	}

	int result = eventLoop.execute_for(duration);
	BOOST_CHECK_EQUAL(counter, timers.size());
	BOOST_CHECK(result == -1);
}

