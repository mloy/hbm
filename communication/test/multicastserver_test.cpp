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
#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>

#include <boost/test/unit_test.hpp>

#include "hbm/communication/multicastserver.h"
#include "hbm/communication/netadapter.h"
#include "hbm/communication/netadapterlist.h"

#include "hbm/sys/eventloop.h"
#include "hbm/sys/timer.h"


static std::string received;
static std::mutex receivedMtx;
static std::condition_variable receivedCnd;

static bool checkReceived()
{
	return !received.empty();
}

static int receiveAndKeep(hbm::communication::MulticastServer* pMcs)
{
	ssize_t result;
	do {
		char buf[1042];
		unsigned int adapterIndex;
		int ttl;
		result = pMcs->receiveTelegram(buf, sizeof(buf), adapterIndex, ttl);
		if(result>0) {
			received = std::string(buf, result);
			receivedCnd.notify_one();
			std::cout << __FUNCTION__ << " '" << received << "'" <<std::endl;
		}
	} while(result>=0);
	return 0;
}

static int receiveAndDiscard(hbm::communication::MulticastServer* pMcs)
{
	ssize_t result;
	do {
		char buf[1042];
		unsigned int adapterIndex;
		int ttl;
		result = pMcs->receiveTelegram(buf, sizeof(buf), adapterIndex, ttl);
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
	fgets(readBuffer, sizeof(readBuffer), pipe);
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
		GetProcessHandleCount(GetCurrentProcess(), &fdCountAfter);
	}
	
#ifdef _WIN32
	GetProcessHandleCount(GetCurrentProcess(), &fdCountAfter);
#else
	pipe = popen(cmd.c_str(), "r");
	fgets(readBuffer, sizeof(readBuffer), pipe);
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

	static const std::string MSG = "testest";
	int result;
	bool signaled;

	hbm::sys::EventLoop eventloop;
	std::thread worker(std::thread(std::bind(&hbm::sys::EventLoop::execute, std::ref(eventloop))));
	hbm::communication::NetadapterList adapters;
	hbm::communication::MulticastServer mcsReceiver(adapters, eventloop);
	hbm::communication::MulticastServer mcsSender(adapters, eventloop);

	result = mcsSender.start(MULTICASTGROUP, UDP_PORT, std::bind(&receiveAndDiscard, std::placeholders::_1));
	BOOST_CHECK_EQUAL(result,0);

	mcsSender.addAllInterfaces();
	mcsSender.setMulticastLoop(true);
	for (unsigned int i=0; i<CYCLECOUNT; ++i)
	{
		received.clear();
		result = mcsReceiver.start(MULTICASTGROUP, UDP_PORT, std::bind(&receiveAndKeep, std::placeholders::_1));
		mcsReceiver.addAllInterfaces();
		mcsSender.send(MSG.c_str(), MSG.length());

		{
			std::unique_lock < std::mutex > lock(receivedMtx);
			signaled = receivedCnd.wait_for(lock, std::chrono::milliseconds(100), checkReceived);
		}

		//std::this_thread::sleep_for(std::chrono::milliseconds(100));
		BOOST_CHECK_EQUAL(signaled, true);
		mcsReceiver.stop();
		BOOST_CHECK_EQUAL(MSG, received);

		std::cout << __FUNCTION__ << " " << i << std::endl;
	}

	mcsSender.stop();

	eventloop.stop();
	worker.join();

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
