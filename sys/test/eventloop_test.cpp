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
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "hbm/sys/eventloop.h"
#include "hbm/sys/timer.h"
#include "hbm/sys/notifier.h"
#include "hbm/exception/exception.hpp"


static unsigned int incrementCount = 0;
static unsigned int incrementLimit = 0;
static std::mutex incrementLimitMtx;
static std::condition_variable incrementLimitCnd;

static void timerEventHandlerIncrement(bool fired, unsigned int& value, bool& canceled)
{
	if (fired) {
		++value;
		canceled = false;
	}
	else {
		canceled = true;
	}
}

static void timerEventHandlerNop(bool )
{
}

static void executionTimerCb(bool fired, hbm::sys::EventLoop& eventloop)
{
	if (fired) {
		eventloop.stop();
	}
}


static void notifierIncrement(unsigned int& value)
{
	++value;
}

static void notifierIncrementCheckLimit()
{
	++incrementCount;
	if (incrementCount==incrementLimit) {
		incrementLimitCnd.notify_one();
	}
}

static bool getLimitReached()
{
	return incrementCount>=incrementLimit;
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

/// event loop might be stopped and started again.
BOOST_AUTO_TEST_CASE(eventloop_restart_test)
{
	unsigned int notificationCount = 0;
	hbm::sys::EventLoop eventLoop;
	
	hbm::sys::Notifier notifier(eventLoop);
	hbm::sys::Timer timer(eventLoop);
	static const std::chrono::milliseconds waitDuration(10);
	
	notifier.notify();
	std::this_thread::sleep_for(waitDuration);
	// event loop is not running yet!
	BOOST_CHECK_EQUAL(notificationCount, 0);


	
	timer.set(waitDuration*2, true, &timerEventHandlerNop);
	notifier.set(std::bind(&notifierIncrement, std::ref(notificationCount)));

	std::thread worker(std::bind(&hbm::sys::EventLoop::execute, std::ref(eventLoop)));
	std::this_thread::sleep_for(waitDuration);
	// Event loop is running. Notification should happen!
	BOOST_CHECK_EQUAL(notificationCount, 1);
	eventLoop.stop();
	worker.join();


	worker = std::thread(std::bind(&hbm::sys::EventLoop::execute, std::ref(eventLoop)));
	notifier.notify();
	
	std::this_thread::sleep_for(waitDuration);

	eventLoop.stop();
	worker.join();
	// 2nd notification after restart of event loop.
	BOOST_CHECK_EQUAL(notificationCount, 2);
}


BOOST_AUTO_TEST_CASE(waitforend_test)
{
	hbm::sys::EventLoop eventLoop;
	hbm::sys::Timer executionTimer(eventLoop);
	static const std::chrono::milliseconds duration(100);

	std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	executionTimer.set(duration, false, std::bind(&executionTimerCb, std::placeholders::_1, std::ref(eventLoop)));
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
		executionTimer.set(duration, false, std::bind(&executionTimerCb, std::placeholders::_1, std::ref(eventLoop)));
		int result = eventLoop.execute();
		std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();
		std::chrono::milliseconds delta = std::chrono::duration_cast < std::chrono::milliseconds > (endTime-startTime);
		executionTimer.cancel();

		BOOST_CHECK_EQUAL(result, 0);
		BOOST_CHECK_GE(delta.count(), duration.count());
	}
}



BOOST_AUTO_TEST_CASE(notify_test)
{
	unsigned int notificationCount = 0;
	static const std::chrono::milliseconds duration(100);
	hbm::sys::EventLoop eventLoop;
	hbm::sys::Notifier notifier(eventLoop);
	notifier.set(std::bind(&notifierIncrement, std::ref(notificationCount)));
	BOOST_CHECK_EQUAL(notificationCount, 0);

	std::thread worker(std::bind(&hbm::sys::EventLoop::execute, &eventLoop));


	static const unsigned int count = 10;
	for(unsigned int i=0; i<count; ++i) {
		notifier.notify();

#ifdef _WIN32
		/// this is important for windows. linux event_fd accumulates events windows is only able to signal one event.
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
	}



	eventLoop.stop();
	worker.join();

	BOOST_CHECK_EQUAL(notificationCount, count);
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
	executionTimer.set(duration, false, std::bind(&executionTimerCb, std::placeholders::_1, std::ref(eventLoop)));
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
	executionTimer.set(duration, false, std::bind(&executionTimerCb, std::placeholders::_1, std::ref(eventLoop)));
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
	executionTimer.set(duration, false, std::bind(&executionTimerCb, std::placeholders::_1, std::ref(eventLoop)));

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

	static const unsigned int restartCount = 10;
	unsigned int counter = 0;
	bool canceled = false;

	std::thread worker(std::bind(&hbm::sys::EventLoop::execute, std::ref(eventLoop)));
	hbm::sys::Timer timer(eventLoop);

	timer.set(std::chrono::milliseconds(duration), false, std::bind(&timerEventHandlerIncrement, std::placeholders::_1, std::ref(counter), std::ref(canceled)));
	std::this_thread::sleep_for(duration / 2);
	timer.cancel();
	BOOST_CHECK_EQUAL(canceled, true);

	for (unsigned int i = 0; i < restartCount; ++i) {
		timer.set(std::chrono::milliseconds(duration), false, std::bind(&timerEventHandlerIncrement, std::placeholders::_1, std::ref(counter), std::ref(canceled)));
		std::this_thread::sleep_for(duration * 2);
	}
	BOOST_CHECK_EQUAL(canceled, false);
	BOOST_CHECK_EQUAL(counter, restartCount);


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
	executionTimer.set(duration, false, std::bind(&executionTimerCb, std::placeholders::_1, std::ref(eventLoop)));
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

BOOST_AUTO_TEST_CASE(addandremoveevents_test)
{
	typedef std::vector < std::unique_ptr < hbm::sys::Notifier > > Notifiers;
	static const unsigned int cycleCount = 10;
	static const unsigned int eventCount = 1000;
	hbm::sys::EventLoop eventLoop;


	Notifiers notifiers;

	incrementLimit = eventCount;
	bool signaled;
	

	std::thread worker = std::thread(std::bind(&hbm::sys::EventLoop::execute, std::ref(eventLoop)));


	for (unsigned int cycle = 0; cycle < cycleCount; ++cycle) {
		incrementCount = 0;
		for (unsigned int i = 0; i < eventCount; ++i) {
			std::unique_ptr < hbm::sys::Notifier >notifier(new hbm::sys::Notifier(eventLoop));
			//notifier->set(std::bind(&notifierIncrement, std::ref(eventCounter)));
			notifier->set(std::bind(&notifierIncrementCheckLimit));
			notifiers.push_back(std::move(notifier));
		}
		for (unsigned int i = 0; i < eventCount; ++i) {
			notifiers[i]->notify();
		}
		
		{
			std::unique_lock < std::mutex > lock(incrementLimitMtx);
			signaled = incrementLimitCnd.wait_for(lock, std::chrono::milliseconds(100), getLimitReached);
		}
		BOOST_CHECK_EQUAL(signaled, true);
		BOOST_CHECK_EQUAL(incrementCount, eventCount);

		notifiers.clear();
	}

	eventLoop.stop();
	worker.join();
}
