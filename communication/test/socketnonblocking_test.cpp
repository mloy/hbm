// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#ifndef _WIN32
#define BOOST_TEST_DYN_LINK
#endif
#define BOOST_TEST_MAIN

#define BOOST_TEST_MODULE socketnonblocking tests

#include <boost/test/unit_test.hpp>

#include <string>
#include <thread>
#include <functional>
#include <memory>


#include "hbm/communication/socketnonblocking.h"
#include "hbm/communication/tcpserver.h"
#include "hbm/communication/test/socketnonblocking_test.h"
#include "hbm/sys/eventloop.h"

namespace hbm {
	namespace communication {
		namespace test {

			static const unsigned int PORT = 22222;


			serverFixture::serverFixture()
				: m_server(m_eventloop)
			{
				BOOST_TEST_MESSAGE("setup Fixture1");
				int result = m_server.start(PORT, 3, std::bind(&serverFixture::acceptCb, this, std::placeholders::_1));
				BOOST_CHECK_MESSAGE(result == 0, strerror(errno));
				m_serverWorker = std::thread(std::bind(&hbm::sys::EventLoop::execute, std::ref(m_eventloop)));
			}

			serverFixture::~serverFixture()
			{
				BOOST_TEST_MESSAGE("teardown Fixture1");
				//m_worker->disconnect();
				m_eventloop.stop();
				m_serverWorker.join();
			}


			void serverFixture::acceptCb(workerSocket_t worker)
			{
				m_worker = std::move(worker);
				m_worker->setDataCb(std::bind(&serverFixture::serverEcho, this));
			}

			int serverFixture::serverEcho()
			{
				char buffer[1024];
				ssize_t result;

				do {
					result = m_worker->receive(buffer, sizeof(buffer));
					if (result>0) {
						result = m_worker->sendBlock(buffer, result, false);
					} else if (result==0) {
						// socket got closed
					} else {
						if ((errno!=EAGAIN) && (errno!=EWOULDBLOCK)) {
							// a real error
						}
					}
				} while (result>0);
				return result;
			}



			int serverFixture::clientReceive(hbm::communication::SocketNonblocking& socket)
			{
				char buffer[1024];
				ssize_t result;

				result = socket.receive(buffer, sizeof(buffer));
				if (result>0) {
					m_answer += buffer;
				}

				return result;
			}


			BOOST_FIXTURE_TEST_SUITE( socket_test, serverFixture )


			BOOST_AUTO_TEST_CASE(echo_test)
			{
				int result;
				const char msg[] = "hallo!";

				hbm::sys::EventLoop eventloop;
				std::thread worker(std::bind(&hbm::sys::EventLoop::execute, std::ref(eventloop)));

				hbm::communication::SocketNonblocking client(eventloop);
				result = client.connect("127.0.0.1", std::to_string(PORT));
				BOOST_CHECK_MESSAGE(result == 0, strerror(errno));
				client.setDataCb(std::bind(&serverFixture::clientReceive, this, std::ref(client)));

				clearAnswer();
				result = client.sendBlock(msg, sizeof(msg), false);
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				BOOST_CHECK_MESSAGE(result == sizeof(msg), strerror(errno));

				client.disconnect();

				BOOST_CHECK_EQUAL(getAnswer(), msg);


				eventloop.stop();
				worker.join();
			}

			BOOST_AUTO_TEST_CASE(writev_test)
			{
				int result;
				static const size_t bufferSize = 100000;
				static const size_t blockCount = 10;
				static const size_t blockSize = bufferSize/blockCount;
				char buffer[bufferSize] = "a";

				hbm::communication::dataBlocks_t dataBlocks;

				for(unsigned int i=0; i<blockCount; ++i) {
					hbm::communication::dataBlock_t dataBlock(&buffer[i*blockSize], blockSize);
					dataBlocks.push_back(dataBlock);
				}

				hbm::sys::EventLoop eventloop;
				std::thread worker(std::bind(&hbm::sys::EventLoop::execute, std::ref(eventloop)));

				hbm::communication::SocketNonblocking client(eventloop);
				result = client.connect("127.0.0.1", std::to_string(PORT));
				BOOST_CHECK_MESSAGE(result == 0, strerror(errno));

				client.setDataCb(std::bind(&serverFixture::clientReceive, this, std::ref(client)));

				clearAnswer();
				result = client.sendBlocks(dataBlocks);
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				client.disconnect();

				BOOST_CHECK_MESSAGE(result == bufferSize, strerror(errno));


				eventloop.stop();
				worker.join();
			}


			BOOST_AUTO_TEST_SUITE_END()
		}
	}
}
