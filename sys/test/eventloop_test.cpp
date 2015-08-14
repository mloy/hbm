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


static void eventHandlerPrint()
{
	std::cout << __FUNCTION__ << std::endl;
}


/// by returning error, the execute() method, that is doing the eventloop, exits
static void timerEventHandlerIncrement(bool fired, unsigned int& value, bool& canceled)
{
	if (fired) {
		++value;
		canceled = false;
	} else {
		canceled = true;
	}
}

/// by returning error, the execute() method, that is doing the eventloop, exits
static void notifierEventHandlerIncrement(unsigned int& value)
{
	++value;
}





/// start the eventloop in a separate thread wait some time and stop it.
BOOST_AUTO_TEST_CASE(stop_test)
{
	hbm::sys::EventLoop eventLoop;

	static const std::chrono::milliseconds waitDuration(300);

	std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	std::thread worker(std::bind(&hbm::sys::EventLoop::execute, std::ref(eventLoop)));
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
	hbm::sys::Timer executionTimer(eventLoop);
	static const std::chrono::milliseconds duration(100);
	

	std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	executionTimer.set(duration, false, std::bind(&hbm::sys::EventLoop::stop, std::ref(eventLoop)));
	int result = eventLoop.execute();
	std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();

	std::chrono::milliseconds delta = std::chrono::duration_cast < std::chrono::milliseconds > (endTime-startTime);

	BOOST_CHECK_EQUAL(result, 0);
	BOOST_CHECK_GE(delta.count(), duration.count()-3);
}

BOOST_AUTO_TEST_CASE(restart_test)
{
	hbm::sys::EventLoop eventLoop;
	hbm::sys::Timer executionTimer(eventLoop);

	static const std::chrono::milliseconds duration(100);

	for (unsigned int i=0; i<10; ++i) {
		std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
		executionTimer.set(duration, false, std::bind(&hbm::sys::EventLoop::stop, std::ref(eventLoop)));
		int result = eventLoop.execute();
		std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();
		std::chrono::milliseconds delta = std::chrono::duration_cast < std::chrono::milliseconds > (endTime-startTime);

		BOOST_CHECK_EQUAL(result, 0);
		BOOST_CHECK_GE(delta.count(), duration.count()-3);
	}
}



BOOST_AUTO_TEST_CASE(notify_test)
{
	unsigned int value = 0;
	static const std::chrono::milliseconds duration(100);
	hbm::sys::EventLoop eventLoop;
	hbm::sys::Notifier notifier(eventLoop);
	notifier.set(std::bind(&notifierEventHandlerIncrement, std::ref(value)));
	BOOST_CHECK_EQUAL(value, 0);

	std::thread worker(std::bind(&hbm::sys::EventLoop::execute, &eventLoop));


	static const unsigned int count = 10;
	for(unsigned int i=0; i<count; ++i) {
		notifier.notify();

#ifdef _WIN32
		/// this is important for windows. linux event_fd accumulates events windows is only able to signal one event.
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
	}


	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	eventLoop.stop();
	worker.join();

	BOOST_CHECK_EQUAL(value, count);
}

BOOST_AUTO_TEST_CASE(oneshottimer_test)
{
	static const std::chrono::milliseconds timerCycle(100);
	static const std::chrono::milliseconds delta(10);
	static const unsigned int timerCount = 10;
	static const std::chrono::milliseconds duration(timerCycle * timerCount);
	hbm::sys::EventLoop eventLoop;

	unsigned int counter = 0;
	bool canceled = false;

	hbm::sys::Timer timer(eventLoop);
	hbm::sys::Timer executionTimer(eventLoop);
	timer.set(timerCycle, false, std::bind(&timerEventHandlerIncrement, std::placeholders::_1, std::ref(counter), std::ref(canceled)));
	executionTimer.set(duration, false, std::bind(&hbm::sys::EventLoop::stop, std::ref(eventLoop)));
	int result = eventLoop.execute();
	BOOST_CHECK_EQUAL(counter, 1);
	BOOST_CHECK_EQUAL(result, 0);


	counter = 0;
	std::thread worker = std::thread(std::bind(&hbm::sys::EventLoop::execute, std::ref(eventLoop)));
	timer.set(timerCycle, false, std::bind(&timerEventHandlerIncrement, std::placeholders::_1, std::ref(counter), std::ref(canceled)));

	std::this_thread::sleep_for(timerCycle+delta);
	BOOST_CHECK_EQUAL(counter, 1);

	timer.set(timerCycle, false, std::bind(&timerEventHandlerIncrement, std::placeholders::_1, std::ref(counter), std::ref(canceled)));

	std::this_thread::sleep_for(timerCycle+delta);
	BOOST_CHECK_EQUAL(counter, 2);

	eventLoop.stop();
	worker.join();
}

BOOST_AUTO_TEST_CASE(cyclictimer_test)
{
	static const unsigned int excpectedMinimum = 10;
	static const unsigned int timerCycle = 100;
	static const unsigned int timerCount = excpectedMinimum+1;
	static const std::chrono::milliseconds duration(timerCycle * timerCount);
	hbm::sys::EventLoop eventLoop;

	unsigned int counter = 0;
	bool canceled = false;

	hbm::sys::Timer cyclicTimer(eventLoop);
	hbm::sys::Timer executionTimer(eventLoop);
	cyclicTimer.set(timerCycle, true, std::bind(&timerEventHandlerIncrement, std::placeholders::_1, std::ref(counter), std::ref(canceled)));
	executionTimer.set(duration, false, std::bind(&hbm::sys::EventLoop::stop, std::ref(eventLoop)));
	int result = eventLoop.execute();

	BOOST_CHECK_GE(counter, excpectedMinimum);
	BOOST_CHECK_EQUAL(result, 0);
}

BOOST_AUTO_TEST_CASE(severaltimerevents_test)
{
	static const unsigned int timerCycle = 100;
	static const unsigned int timerCount = 10;
	static const std::chrono::milliseconds duration(timerCycle * timerCount);
	hbm::sys::EventLoop eventLoop;

	unsigned int counter = 0;
	bool canceled = false;

	hbm::sys::Timer cyclicTimer(eventLoop);

	std::thread worker(std::bind(&hbm::sys::EventLoop::execute, std::ref(eventLoop)));

	for (unsigned int i = 0; i < timerCount; ++i) {
		cyclicTimer.set(timerCycle, false, std::bind(&timerEventHandlerIncrement, std::placeholders::_1, std::ref(counter), std::ref(canceled)));
		std::this_thread::sleep_for(std::chrono::milliseconds(timerCycle + 10));
	}

	eventLoop.stop();
	worker.join();

	BOOST_CHECK_EQUAL(counter, timerCount);
}


BOOST_AUTO_TEST_CASE(canceltimer_test)
{
	static const unsigned int timerCycle = 100;
	static const unsigned int timerCount = 10;
	static const std::chrono::milliseconds duration(timerCycle * timerCount);
	hbm::sys::EventLoop eventLoop;

	unsigned int counter = 0;
	bool canceled = false;

	hbm::sys::Timer timer(eventLoop);
	hbm::sys::Timer executionTimer(eventLoop);
	timer.set(timerCycle, false, std::bind(&timerEventHandlerIncrement, std::placeholders::_1, std::ref(counter), std::ref(canceled)));
	executionTimer.set(duration, false, std::bind(&hbm::sys::EventLoop::stop, std::ref(eventLoop)));

	std::thread worker = std::thread(std::bind(&hbm::sys::EventLoop::execute, std::ref(eventLoop)));

	timer.cancel();

	worker.join();

	BOOST_CHECK_EQUAL(canceled, true);
	BOOST_CHECK_EQUAL(counter, 0);
}

BOOST_AUTO_TEST_CASE(restart_timer_test)
{
	hbm::sys::EventLoop eventLoop;

	static const std::chrono::milliseconds duration(100);

	unsigned int counter = 0;
	bool canceled = false;

	std::thread worker(std::bind(&hbm::sys::EventLoop::execute, std::ref(eventLoop)));
	hbm::sys::Timer timer(eventLoop);

	timer.set(std::chrono::milliseconds(duration), false, std::bind(&timerEventHandlerIncrement, std::placeholders::_1, std::ref(counter), std::ref(canceled)));
	std::this_thread::sleep_for(duration / 2);
	timer.cancel();
	BOOST_CHECK_EQUAL(canceled, true);
	timer.set(std::chrono::milliseconds(duration), false, std::bind(&timerEventHandlerIncrement, std::placeholders::_1, std::ref(counter), std::ref(canceled)));
	std::this_thread::sleep_for(duration * 2);

	BOOST_CHECK_EQUAL(canceled, false);
	BOOST_CHECK_EQUAL(counter, 1);

	eventLoop.stop();
	worker.join();
}

BOOST_AUTO_TEST_CASE(retrigger_timer_test)
{
	hbm::sys::EventLoop eventLoop;

	static const std::chrono::milliseconds duration(50);

	unsigned int counter = 0;
	bool canceled = false;

	std::thread worker(std::bind(&hbm::sys::EventLoop::execute, std::ref(eventLoop)));
	hbm::sys::Timer timer(eventLoop);

	timer.set(std::chrono::milliseconds(duration), false, std::bind(&timerEventHandlerIncrement, std::placeholders::_1, std::ref(counter), std::ref(canceled)));


	// we retrigger the timer before it does signal!
	std::this_thread::sleep_for(duration / 2);
	timer.set(std::chrono::milliseconds(duration * 2), false, std::bind(&timerEventHandlerIncrement, std::placeholders::_1, std::ref(counter), std::ref(canceled)));


	std::this_thread::sleep_for(duration);
	// the first trigger would be signaled here. This should not be the case!
	BOOST_CHECK_EQUAL(counter, 0);


	std::this_thread::sleep_for(duration * 2);
	// the second trigger should be signaled here.
	BOOST_CHECK_EQUAL(counter, 1);


	// start timer one more time and make sure event gets signaled
	timer.set(std::chrono::milliseconds(duration / 2), false, std::bind(&timerEventHandlerIncrement, std::placeholders::_1, std::ref(counter), std::ref(canceled)));
	std::this_thread::sleep_for(duration);
	BOOST_CHECK_EQUAL(counter, 2);

	eventLoop.stop();
	worker.join();
}




BOOST_AUTO_TEST_CASE(removenotifier_test)
{
	static const unsigned int timerCycle = 100;
	static const unsigned int timerCount = 10;
	static const std::chrono::milliseconds duration(timerCycle * timerCount);
	hbm::sys::EventLoop eventLoop;
	hbm::sys::Timer executionTimer(eventLoop);

	unsigned int counter = 0;
	bool canceled = false;
	executionTimer.set(duration, false, std::bind(&hbm::sys::EventLoop::stop, std::ref(eventLoop)));
	std::thread worker = std::thread(std::bind(&hbm::sys::EventLoop::execute, std::ref(eventLoop)));

	{
		// leaving this scope leads to destruction of the timer and the removal from the event loop
		hbm::sys::Timer cyclicTimer(eventLoop);
		cyclicTimer.set(timerCycle, true, std::bind(&timerEventHandlerIncrement, std::placeholders::_1, std::ref(counter), std::ref(canceled)));
		std::this_thread::sleep_for(std::chrono::milliseconds(timerCycle * timerCount / 2));
	}

	worker.join();

	BOOST_CHECK_LT(counter, timerCount);
}
