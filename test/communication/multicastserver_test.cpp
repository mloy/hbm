// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#ifndef _WIN32
#define BOOST_TEST_DYN_LINK
#endif
#define BOOST_TEST_MAIN

#define BOOST_TEST_MODULE communication tests


#include <chrono>
#include <functional>
#include <future>
#include <iostream>

#include <boost/test/unit_test.hpp>

#include "hbm/communication/multicastserver.h"
#include "hbm/communication/netadapter.h"
#include "hbm/communication/netadapterlist.h"

#include "hbm/sys/eventloop.h"
#include "hbm/sys/timer.h"


static int receiveAndKeep(hbm::communication::MulticastServer& mcs, std::promise < std::string >& received)
{
	ssize_t result;
	do {
		char buf[1024];
		unsigned int adapterIndex;
		int ttl;
		result = mcs.receiveTelegram(buf, sizeof(buf), adapterIndex, ttl);
		if (result>0) {
			// we may receive the same message several times because there might be several interfaces used for sending
			std::string receivedData(buf, static_cast< size_t >(result));
			received.set_value(receivedData);
			std::cout << __FUNCTION__ << " '" << receivedData << "'" <<std::endl;
		}
	} while(result>=0);
	return 0;
}

static int receiveAndDiscard(hbm::communication::MulticastServer& mcs)
{
	ssize_t result;
	char buf[1024];
	unsigned int adapterIndex;
	int ttl;
	do {
		result = mcs.receiveTelegram(buf, sizeof(buf), adapterIndex, ttl);
	} while(result>=0);
	return 0;
}

static void executionTimerCb(bool fired, hbm::sys::EventLoop& eventloop)
{
	if (fired) {
		eventloop.stop();
	}
}


BOOST_AUTO_TEST_CASE(check_leak)
{
	static const std::chrono::milliseconds waitDuration(1);

	char readBuffer[1024] = "";

	
	hbm::communication::NetadapterList adapterlist;

#ifdef _WIN32
	DWORD fdCountBefore;
	DWORD fdCountAfter;


	{
		// Do one create and destruct cycle under windows before retrieving the number of handles before. This is important beacuse a lot of handles will be created on the first run.
		hbm::sys::EventLoop eventloop;
		hbm::sys::Timer executionTimer(eventloop);
		hbm::communication::MulticastServer mcs(adapterlist, eventloop);
		executionTimer.set(waitDuration, false, std::bind(&executionTimerCb, std::placeholders::_1, std::ref(eventloop)));
		eventloop.execute();

		mcs.stop();
	}
	GetProcessHandleCount(GetCurrentProcess(), &fdCountBefore);

#else
	unsigned int fdCountBefore;
	unsigned int fdCountAfter;

	FILE* pipe;
	std::string cmd;
	pid_t processId = getpid();

	// the numbe of file descriptors of this process
	cmd = "ls -1 /proc/" + std::to_string(processId) + "/fd | wc -l";
	pipe = popen(cmd.c_str(), "r");
	char* pResultString = fgets(readBuffer, sizeof(readBuffer), pipe);
	BOOST_CHECK(pResultString);
	fdCountBefore = std::stoul(readBuffer);
	fclose(pipe);
#endif


	for (unsigned cycle = 0; cycle<10; ++cycle) {
		hbm::sys::EventLoop eventloop;
		hbm::sys::Timer executionTimer(eventloop);
		hbm::communication::MulticastServer mcs(adapterlist, eventloop);
		executionTimer.set(waitDuration, false, std::bind(&executionTimerCb, std::placeholders::_1, std::ref(eventloop)));
		eventloop.execute();

		mcs.stop();
	}
	
#ifdef _WIN32
	GetProcessHandleCount(GetCurrentProcess(), &fdCountAfter);
#else
	pipe = popen(cmd.c_str(), "r");
	pResultString = fgets(readBuffer, sizeof(readBuffer), pipe);
	BOOST_CHECK(pResultString);
	fdCountAfter = std::stoul(readBuffer);
	fclose(pipe);
#endif

	BOOST_CHECK_EQUAL(fdCountBefore, fdCountAfter);
}

BOOST_AUTO_TEST_CASE(start_send_stop_test)
{
	static const char MULTICASTGROUP[] = "239.255.77.177";
	static const unsigned int UDP_PORT = 22222;
	static const unsigned int CYCLECOUNT = 100;

	static const std::string MSG = "test1test2";
	int result;


	hbm::sys::EventLoop eventloop;
	std::future < int > workerResult = std::async(std::launch::async, std::bind(&hbm::sys::EventLoop::execute, std::ref(eventloop)));
	hbm::communication::NetadapterList adapters;
	hbm::communication::MulticastServer mcsReceiver(adapters, eventloop);
	hbm::communication::MulticastServer mcsSender(adapters, eventloop);
	hbm::communication::Netadapter firstadapter = adapters.get().begin()->second;

	result = mcsSender.start(MULTICASTGROUP, UDP_PORT, std::bind(&receiveAndDiscard, std::placeholders::_1));
	BOOST_CHECK_EQUAL(result,0);

	mcsSender.setMulticastLoop(true);
	for (unsigned int i=0; i<CYCLECOUNT; ++i) {
		std::promise < std::string > received;
		std::future < std::string > f = received.get_future();

		result = mcsReceiver.start(MULTICASTGROUP, UDP_PORT, std::bind(&receiveAndKeep, std::placeholders::_1, std::ref(received)));
		BOOST_CHECK_EQUAL(result, 0);
		mcsReceiver.addAllInterfaces();
		mcsSender.send(MSG.c_str(), MSG.length());

		std::future_status fStatus = f.wait_for(std::chrono::milliseconds(100));
		BOOST_CHECK(fStatus == std::future_status::ready);
		mcsReceiver.stop();
		BOOST_CHECK_EQUAL(MSG, f.get());

		std::cout << __FUNCTION__ << " " << i << std::endl;
	}

	mcsSender.stop();

	eventloop.stop();
	workerResult.wait();

	std::cout << __FUNCTION__ << " done" << std::endl;
}

BOOST_AUTO_TEST_CASE(test_join)
{
	static const char MULTICASTGROUP[] = "239.255.77.177";
	static const unsigned int UDP_PORT = 22222;

	hbm::sys::EventLoop eventloop;
	hbm::communication::NetadapterList adapters;
	hbm::communication::MulticastServer server(adapters, eventloop);
	int result = server.start(MULTICASTGROUP, UDP_PORT, std::bind(&receiveAndDiscard, std::placeholders::_1));
	BOOST_REQUIRE(result == 0);

	result = server.addInterface("bla");
	BOOST_CHECK(result < 0);

	result = server.addInterface("127.0.0.1");
	BOOST_CHECK(result >= 0);

	server.stop();
}
