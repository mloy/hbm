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
#include <future>
#include <thread>
#include <functional>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "hbm/sys/eventloop.h"
#include "hbm/sys/timer.h"
#include "hbm/sys/notifier.h"
#include "hbm/sys/executecommand.h"
#include "hbm/exception/exception.hpp"


static int dummyCb()
{
	return 0;
}

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

static void recursiveNotifierIncrement(hbm::sys::Notifier& notifier, unsigned int& value)
{
	++value;
	notifier.notify();
}

static void decrementCounter(unsigned int &counter, std::promise <void > &notifier)
{
	--counter;
	if (counter==0) {
		notifier.set_value();
	}
}


BOOST_AUTO_TEST_CASE(check_leak)
{
	static const std::chrono::milliseconds waitDuration(1);

	char readBuffer[1024] = "";


#ifdef _WIN32
	DWORD fdCountBefore;
	DWORD fdCountAfter;

	{
		// Do one create and destruct cycle under windows before retrieving the number of handles before. This is important beacuse a lot of handles will be created on the first run.
		hbm::sys::EventLoop eventloop;
		hbm::sys::Timer executionTimer(eventloop);
		hbm::sys::Notifier notifier(eventloop);
		executionTimer.set(waitDuration, false, std::bind(&executionTimerCb, std::placeholders::_1, std::ref(eventloop)));
		notifier.notify();
		eventloop.execute();
	}
	GetProcessHandleCount(GetCurrentProcess(), &fdCountBefore);
#else
	unsigned int fdCountBefore;
	unsigned int fdCountAfter;
	pid_t processId = getpid();
	FILE* pipe;
	std::string cmd;
	// the numbe of file descriptors of this process
	cmd = "ls -1 /proc/" + std::to_string(processId) + "/fd | wc -l";
	pipe = popen(cmd.c_str(), "r");
	char* pResultString = fgets(readBuffer, sizeof(readBuffer), pipe);
	BOOST_CHECK(pResultString);
	fdCountBefore = static_cast < unsigned int > (std::stoul(readBuffer));
	fclose(pipe);
#endif

	for (unsigned cycle = 0; cycle<10; ++cycle) {
		hbm::sys::EventLoop eventloop;
		hbm::sys::Timer executionTimer(eventloop);
		hbm::sys::Notifier notifier(eventloop);
		executionTimer.set(waitDuration, false, std::bind(&executionTimerCb, std::placeholders::_1, std::ref(eventloop)));
		notifier.notify();
		eventloop.execute();
	}
	
#ifdef _WIN32
	GetProcessHandleCount(GetCurrentProcess(), &fdCountAfter);
#else
	pipe = popen(cmd.c_str(), "r");
	pResultString = fgets(readBuffer, sizeof(readBuffer), pipe);
	BOOST_CHECK(pResultString);
	fdCountAfter = static_cast < unsigned int > (std::stoul(readBuffer));
	fclose(pipe);
#endif

	BOOST_CHECK_EQUAL(fdCountBefore, fdCountAfter);
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
		// Under windows we need a delta here. Under linux is the result is exact!
		BOOST_CHECK_LE(abs(duration.count() - delta.count()), 1);
	}
}



BOOST_AUTO_TEST_CASE(notify_test)
{
	unsigned int notificationCount = 0;
	//static const std::chrono::milliseconds duration(100);
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

BOOST_AUTO_TEST_CASE(recursive_notification_test)
{
	unsigned int notificationCount = 0;
	hbm::sys::EventLoop eventLoop;
	hbm::sys::Notifier notifier(eventLoop);
	notifier.set(std::bind(&recursiveNotifierIncrement, std::ref(notifier), std::ref(notificationCount)));
	BOOST_CHECK_EQUAL(notificationCount, 0);

	std::thread worker(std::bind(&hbm::sys::EventLoop::execute, &eventLoop));


	notifier.notify();

	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	eventLoop.stop();
	worker.join();

	BOOST_CHECK_GT(notificationCount, 1);
}

BOOST_AUTO_TEST_CASE(multiple_event_test)
{
	static const unsigned int NOTIFIER_COUNT = 10;
	unsigned int notificationCount = 0;
	hbm::sys::EventLoop eventLoop;

	std::vector < std::unique_ptr < hbm::sys::Notifier > > notifiers;
	for (unsigned int i=0; i<NOTIFIER_COUNT; ++i) {
		auto notifier = std::make_unique < hbm::sys::Notifier > (eventLoop);
		notifier->set(std::bind(&notifierIncrement, std::ref(notificationCount)));
		notifier->notify();
		notifiers.emplace_back(std::move(notifier));
	}

	BOOST_CHECK_EQUAL(notificationCount, 0);

	std::thread worker(std::bind(&hbm::sys::EventLoop::execute, &eventLoop));

	eventLoop.stop();
	worker.join();

	BOOST_CHECK_EQUAL(notificationCount, NOTIFIER_COUNT);
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

BOOST_AUTO_TEST_CASE(several_timer_events_test)
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

	static const std::chrono::milliseconds duration(50);

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

BOOST_AUTO_TEST_CASE(add_and_remove_event_test)
{
	hbm::sys::EventLoop eventLoop;
	int result;

	result = eventLoop.addEvent(1, &dummyCb);
	BOOST_CHECK_EQUAL(result, 0);
	// overwriting existing is allowed
	result = eventLoop.addEvent(1, &dummyCb);
	BOOST_CHECK_EQUAL(result, 0);
	result = eventLoop.eraseEvent(1);
	BOOST_CHECK_EQUAL(result, 0);
	// removing non existent should fail
	result = eventLoop.eraseEvent(1);
	BOOST_CHECK_EQUAL(result, -1);

	// there was a bug were input event was not removed if input event and output event were set!
	result = eventLoop.addEvent(1, &dummyCb);
	result = eventLoop.addOutEvent(1, &dummyCb);
	result = eventLoop.eraseEvent(1);
	BOOST_CHECK_EQUAL(result, 0);
	// removing non existent should fail
	result = eventLoop.eraseEvent(1);
	BOOST_CHECK_EQUAL(result, -1);

	// the same with trying to erase the output event
	result = eventLoop.addEvent(1, &dummyCb);
	result = eventLoop.addOutEvent(1, &dummyCb);
	result = eventLoop.eraseOutEvent(1);
	BOOST_CHECK_EQUAL(result, 0);
	// removing non existent should fail
	result = eventLoop.eraseOutEvent(1);
	BOOST_CHECK_EQUAL(result, -1);
}
BOOST_AUTO_TEST_CASE(add_and_remove_many_events_test)
{
	typedef std::vector < std::unique_ptr < hbm::sys::Notifier > > Notifiers;
	static const unsigned int cycleCount = 10;
	static const unsigned int notifierCount = 1000;
	hbm::sys::EventLoop eventLoop;


	Notifiers notifiers;


#ifndef _WIN32
	// under windows, the 1st parameter is a complex parameter
	// invalid function pointer
	int result = eventLoop.addEvent(0, nullptr);
	BOOST_CHECK_EQUAL(result, -1);

	// invalid file descriptor
	result = eventLoop.addEvent(-1, &dummyCb);
	BOOST_CHECK_EQUAL(result, -1);
#endif

	std::thread worker = std::thread(std::bind(&hbm::sys::EventLoop::execute, std::ref(eventLoop)));
	unsigned int counter = notifierCount;
	std::promise < void > promise;
	auto f = promise.get_future();


	for (unsigned int cycle = 0; cycle < cycleCount; ++cycle) {
		for (unsigned int i = 0; i < notifierCount; ++i) {
			auto notifier = std::make_unique < hbm::sys::Notifier > (eventLoop);
			notifier->set(std::bind(&decrementCounter, std::ref(counter), std::ref(promise)));
			notifiers.emplace_back(std::move(notifier));
		}
		for (unsigned int i = 0; i < notifierCount; ++i) {
			notifiers[i]->notify();
		}
		std::future_status status = f.wait_for(std::chrono::milliseconds(100));
		BOOST_CHECK(status==std::future_status::ready);
		notifiers.clear();
	}

	eventLoop.stop();
	worker.join();
}
