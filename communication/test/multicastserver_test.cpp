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
