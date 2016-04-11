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
#include "hbm/sys/timer.h"


namespace hbm {
	namespace communication {
		namespace test {

			static const unsigned int PORT = 22222;

			static void executionTimerCb(bool fired, hbm::sys::EventLoop& eventloop)
			{
				if (fired) {
					eventloop.stop();
				}
			}

			serverFixture::serverFixture()
				: m_server(m_eventloop)
			{
				BOOST_TEST_MESSAGE("setup Fixture1");
				int result = m_server.start(PORT, 3, std::bind(&serverFixture::acceptCb, this, std::placeholders::_1));
				BOOST_CHECK_MESSAGE(result == 0, strerror(errno));
			}

			serverFixture::~serverFixture()
			{
				BOOST_TEST_MESSAGE("teardown serverFixture");
				stop();
			}

			size_t serverFixture::getClientCount() const
			{
				return m_workers.size();
			}
			
			void serverFixture::start()
			{
				m_serverWorker = std::thread(std::bind(&hbm::sys::EventLoop::execute, std::ref(m_eventloop)));
			}

			void serverFixture::stop()
			{
				m_eventloop.stop();
				if (m_serverWorker.joinable()) {
					m_serverWorker.join();
				}
			}
			

			void serverFixture::acceptCb(workerSocket_t worker)
			{
				static int clientId = 0;
				++clientId;
				m_workers[clientId] = std::move(worker);
				m_workers[clientId]->setDataCb(std::bind(&serverFixture::serverEcho, this, clientId));
			}

			ssize_t serverFixture::serverEcho(int clientId)
			{
				char buffer[1024];
				ssize_t result;

				do {
					result = m_workers[clientId]->receive(buffer, sizeof(buffer));
					if (result>0) {
						result = m_workers[clientId]->sendBlock(buffer, result, false);
					} else if (result==0) {
						// socket got closed
						m_workers.erase(clientId);
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

			int serverFixture::clientReceiveTarget(hbm::communication::SocketNonblocking& socket, std::string& target)
			{
				char buffer[1024];
				ssize_t result;

				result = socket.receive(buffer, sizeof(buffer));
				if (result>0) {
					target += buffer;
				}

				return result;
			}
			

			int serverFixture::clientReceiveSingleBytes(hbm::communication::SocketNonblocking& socket)
			{
				char buffer;
				ssize_t result;

				result = socket.receive(&buffer, sizeof(buffer));
				if (result > 0) {
					if (buffer != '\0') {
						m_answer += buffer;
					}
				}

				return result;
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
				hbm::communication::SocketNonblocking socket(eventloop);
				executionTimer.set(waitDuration, false, std::bind(&executionTimerCb, std::placeholders::_1, std::ref(eventloop)));
				eventloop.execute();
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
				hbm::communication::SocketNonblocking socket(eventloop);
				executionTimer.set(waitDuration, false, std::bind(&executionTimerCb, std::placeholders::_1, std::ref(eventloop)));
				eventloop.execute();
			}
			
#ifdef _WIN32
			GetProcessHandleCount( GetCurrentProcess(), &fdCountAfter);
#else
			pipe = popen(cmd.c_str(), "r");
			fgets(readBuffer, sizeof(readBuffer), pipe);
			fdCountAfter = std::stoul(readBuffer);
			fclose(pipe);
#endif
		
			BOOST_CHECK_EQUAL(fdCountBefore, fdCountAfter);
		}
		
			BOOST_FIXTURE_TEST_SUITE( socket_test, serverFixture )


			BOOST_AUTO_TEST_CASE(echo_test)
			{
				int result;
				static const char msg[] = "hallo";
				static const char msg2[] = "!";

				start();

				hbm::communication::SocketNonblocking client(m_eventloop);
				client.setDataCb(std::bind(&serverFixture::clientReceiveSingleBytes, this, std::placeholders::_1));
				result = client.connect("127.0.0.1", std::to_string(PORT));
				BOOST_CHECK_MESSAGE(result == 0, strerror(errno));

				clearAnswer();
				result = client.sendBlock(msg, sizeof(msg), false);
				BOOST_CHECK_MESSAGE(result == sizeof(msg), strerror(errno));
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				std::string answer = getAnswer();
				BOOST_CHECK_EQUAL(answer, msg);

				clearAnswer();
				result = client.sendBlock(msg2, sizeof(msg2), false);
				BOOST_CHECK_MESSAGE(result == sizeof(msg2), strerror(errno));
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				answer = getAnswer();
				BOOST_CHECK_EQUAL(answer, msg2);


				clearAnswer();
				result = client.sendBlock(msg, sizeof(msg), false);
				result = client.sendBlock(msg2, sizeof(msg2), false);

				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				answer = getAnswer();
				BOOST_CHECK_EQUAL(answer, std::string(msg) + msg2);

				client.disconnect();



				stop();
			}
			
//			BOOST_AUTO_TEST_CASE(edge_trigger_test)
//			{
//				static const size_t clientCount = 10;
//				int result;
//				static const std::string msgPrefix = "hallo";


//				std::vector < std::string > targets(clientCount);
//				std::vector < std::unique_ptr < hbm::communication::SocketNonblocking > > clients;
				
//				for (size_t clientIndex=0; clientIndex<clientCount; ++clientIndex) {
//					clients.push_back( std::unique_ptr < hbm::communication::SocketNonblocking >(new hbm::communication::SocketNonblocking(m_eventloop)));
//				}
				
//				unsigned int index = 0;
//				for (auto iter = clients.begin(); iter!=clients.end(); ++iter) {
//					(*iter)->setDataCb(std::bind(&serverFixture::clientReceiveTarget, this, std::placeholders::_1, std::ref(targets[index])));
//					result = (*iter)->connect("127.0.0.1", std::to_string(PORT));
//					BOOST_CHECK_MESSAGE(result == 0, strerror(errno));
//					index++;
//				}
				
				
//				start();
				
////				index = 0;
////				for (auto iter = clients.begin(); iter!=clients.end(); ++iter) {
////					std::string msg = msgPrefix + std::to_string(index);
////					result = (*iter)->sendBlock(msg.c_str(), msg.length(), false);
////					BOOST_CHECK_MESSAGE(static_cast < size_t > (result) == msg.length(), strerror(errno));
////					index++;
////				}

//				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				
//				size_t realClientCount = getClientCount();
//				BOOST_CHECK_EQUAL(clientCount, realClientCount);
//				stop();
//			}

			BOOST_AUTO_TEST_CASE(multiclient_echo_test)
			{
				static const size_t clientCount = 10;
				int result;
				static const std::string msgPrefix = "hallo";

				start();

				std::vector < std::unique_ptr < hbm::communication::SocketNonblocking > > clients;
				
				for (size_t clientIndex=0; clientIndex<clientCount; ++clientIndex) {
					clients.push_back( std::unique_ptr < hbm::communication::SocketNonblocking >(new hbm::communication::SocketNonblocking(m_eventloop)));
				}
				
				unsigned int index = 0;
				for (auto iter = clients.begin(); iter!=clients.end(); ++iter) {
					std::string msg = msgPrefix + std::to_string(index++);
					result = (*iter)->connect("127.0.0.1", std::to_string(PORT));
					BOOST_CHECK_MESSAGE(result == 0, strerror(errno));
					(*iter)->setDataCb(std::bind(&serverFixture::clientReceiveSingleBytes, this, std::placeholders::_1));
					
					clearAnswer();
					result = (*iter)->sendBlock(msg.c_str(), msg.length(), false);
					BOOST_CHECK_MESSAGE(static_cast < size_t > (result) == msg.length(), strerror(errno));
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					std::string answer = getAnswer();
					BOOST_CHECK_EQUAL(msg, answer);
				}

				for (auto iter = clients.begin(); iter!=clients.end(); ++iter) {
					(*iter)->disconnect();
				}


				stop();
			}
			
			
#ifndef _WIN32
			// under windows tcpserver does not support ipv6 yet
			BOOST_AUTO_TEST_CASE(echo_test_ipv6)
			{
				int result;
				static const char msg[] = "hallo";

				start();

				hbm::communication::SocketNonblocking client(m_eventloop);
				result = client.connect("::1", std::to_string(PORT));
				BOOST_CHECK_MESSAGE(result == 0, strerror(errno));
				client.setDataCb(std::bind(&serverFixture::clientReceiveSingleBytes, this, std::placeholders::_1));

				clearAnswer();
				result = client.sendBlock(msg, sizeof(msg), false);
				BOOST_CHECK_MESSAGE(result == sizeof(msg), strerror(errno));
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				std::string answer = getAnswer();
				BOOST_CHECK_EQUAL(answer, msg);

				client.disconnect();


				stop();
			}
#endif

			BOOST_AUTO_TEST_CASE(setting_data_callback)
			{
				int result;
				static const char msg[] = "hallo";

				start();

				hbm::communication::SocketNonblocking client(m_eventloop);
				result = client.connect("127.0.0.1", std::to_string(PORT));
				BOOST_CHECK_MESSAGE(result == 0, strerror(errno));
				client.setDataCb(std::bind(&serverFixture::clientReceiveSingleBytes, this, std::placeholders::_1));

				clearAnswer();
				result = client.sendBlock(msg, sizeof(msg), false);
				BOOST_CHECK_MESSAGE(result == sizeof(msg), strerror(errno));
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				std::string answer = getAnswer();
				BOOST_CHECK_EQUAL(answer, msg);

				// set callback function again
				client.setDataCb(std::bind(&serverFixture::clientReceive, this, std::placeholders::_1));
				clearAnswer();
				result = client.sendBlock(msg, sizeof(msg), false);

				std::this_thread::sleep_for(std::chrono::milliseconds(100));

				BOOST_CHECK_MESSAGE(result == sizeof(msg), strerror(errno));
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				answer = getAnswer();
				clearAnswer();
				BOOST_CHECK_EQUAL(answer, msg);

				client.disconnect();



				stop();
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

				start();

				hbm::communication::SocketNonblocking client(m_eventloop);
				result = client.connect("127.0.0.1", std::to_string(PORT));
				BOOST_CHECK_MESSAGE(result == 0, strerror(errno));

				client.setDataCb(std::bind(&serverFixture::clientReceive, this, std::placeholders::_1));

				clearAnswer();
				result = client.sendBlocks(dataBlocks);
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				client.disconnect();

				BOOST_CHECK_MESSAGE(result == bufferSize, strerror(errno));


				stop();
			}


			BOOST_AUTO_TEST_SUITE_END()
		}
	}
}
