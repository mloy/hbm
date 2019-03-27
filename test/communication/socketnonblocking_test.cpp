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
#include "socketnonblocking_test.h"
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
			

			void serverFixture::acceptCb(clientSocket_t worker)
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
#ifdef _WIN32
						int retVal = WSAGetLastError();
						if ((retVal != WSAEWOULDBLOCK) && (retVal != ERROR_IO_PENDING) && (retVal != WSAEINPROGRESS)) {
#else
						if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
#endif


							// a real error
							m_workers.erase(clientId);
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

			int serverFixture::clientNotify(unsigned int &count)
			{
				++count;
				return 0;
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
			if(fgets(readBuffer, sizeof(readBuffer), pipe)==NULL) {
				BOOST_ASSERT("Could not get number of fds of process");
			}
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
			if(fgets(readBuffer, sizeof(readBuffer), pipe)==NULL) {
				BOOST_ASSERT("Could not get number of fds of process");
			}
			fdCountAfter = std::stoul(readBuffer);
			fclose(pipe);
#endif
		
			BOOST_CHECK_EQUAL(fdCountBefore, fdCountAfter);
		}
		
			BOOST_FIXTURE_TEST_SUITE( socket_test, serverFixture )

			BOOST_AUTO_TEST_CASE(connect_test)
			{
				int result;
				start();
				hbm::communication::SocketNonblocking client(m_eventloop);
#ifndef _WIN32
				static const char server[] = "::1";
#else
				static const char server[] = "127.0.0.1";

#endif
				for (unsigned int cycleIndex = 0; cycleIndex < 1000; ++cycleIndex) {
					result = client.connect(server, std::to_string(PORT));
					BOOST_CHECK_MESSAGE(result == 0, strerror(errno));

					client.disconnect();
				}

				// wrong port! Should fail
				result = client.connect(server, std::to_string(PORT + 1));
				BOOST_CHECK_MESSAGE(result == -1, strerror(errno));
			}


			BOOST_AUTO_TEST_CASE(send_recv_test)
			{
				int result;
				static const char msg[] = "hallo";
				char response[1024];

				start();

				hbm::communication::SocketNonblocking client(m_eventloop);
				result = client.connect("127.0.0.1", std::to_string(PORT));
				BOOST_CHECK_MESSAGE(result == 0, strerror(errno));

				result = client.send(msg, sizeof(msg), false);
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				BOOST_CHECK_MESSAGE(result == sizeof(msg), strerror(errno));
				result = client.receive(response, sizeof(msg));
				BOOST_CHECK_MESSAGE(result == sizeof(msg), strerror(errno));
				BOOST_CHECK_EQUAL(response, msg);

				result = client.send(msg, sizeof(msg), false);
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				BOOST_CHECK_MESSAGE(result == sizeof(msg), strerror(errno));
				for( unsigned int cycleIndex=0; cycleIndex<sizeof(msg); ++cycleIndex) {
					result = client.receive(response, 1);
					BOOST_CHECK_MESSAGE(result == 1, strerror(errno));
					BOOST_CHECK_EQUAL(response[0], msg[cycleIndex]);
				}
				result = client.receive(response, 1);
				BOOST_CHECK_EQUAL(result, -1);

				client.disconnect();


				stop();
			}

			BOOST_AUTO_TEST_CASE(sendblock_recvblock_test)
			{
				int result;
				static const char msg[] = "hallo";
				char response[1024];

				start();

				hbm::communication::SocketNonblocking client(m_eventloop);
				//client.setDataCb(std::bind(&serverFixture::clientReceiveSingleBytes, this, std::placeholders::_1));
				result = client.connect("127.0.0.1", std::to_string(PORT));
				BOOST_CHECK_MESSAGE(result == 0, strerror(errno));

				for( unsigned int cycleIndex=0; cycleIndex<100; ++cycleIndex) {
					result = client.sendBlock(msg, sizeof(msg), false);
					BOOST_CHECK_MESSAGE(result == sizeof(msg), strerror(errno));
					result = client.receiveComplete(response, sizeof(msg), 100);
					BOOST_CHECK_MESSAGE(result == sizeof(msg), strerror(errno));
					BOOST_CHECK_EQUAL(response, msg);
				}

				result = client.sendBlock(msg, sizeof(msg), false);
				BOOST_CHECK_MESSAGE(result == sizeof(msg), strerror(errno));
				for( unsigned int cycleIndex=0; cycleIndex<sizeof(msg); ++cycleIndex) {
					result = client.receiveComplete(response, 1, 100);
					BOOST_CHECK_MESSAGE(result == 1, strerror(errno));
					BOOST_CHECK_EQUAL(response[0], msg[cycleIndex]);
				}
				result = client.receiveComplete(response, 1, 100);
				BOOST_CHECK_EQUAL(result, -1);


				// force timeout!
				result = client.receiveComplete(response, sizeof(msg), 100);
				BOOST_CHECK_EQUAL(result, -1);

				// nothing to read and no time to wait
				result = client.receiveComplete(response, sizeof(msg), 0);
				BOOST_CHECK_EQUAL(result, -1);

				client.disconnect();


				stop();
			}
			

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
				for (auto &iter: clients) {
					std::string msg = msgPrefix + std::to_string(index++);
					result = iter->connect("127.0.0.1", std::to_string(PORT));
					BOOST_CHECK_MESSAGE(result == 0, strerror(errno));
					iter->setDataCb(std::bind(&serverFixture::clientReceiveSingleBytes, this, std::placeholders::_1));
					
					clearAnswer();
					result = iter->sendBlock(msg.c_str(), msg.length(), false);
					BOOST_CHECK_MESSAGE(static_cast < size_t > (result) == msg.length(), strerror(errno));
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					std::string answer = getAnswer();
					BOOST_CHECK_EQUAL(msg, answer);
				}

				for (auto &iter: clients) {
					iter->disconnect();
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


			BOOST_AUTO_TEST_CASE(writev_test_list)
			{
				int result;
				static const size_t bufferSize = 100000;
				static const size_t blockCount = 10;
				static const size_t blockSize = bufferSize/blockCount;
				char buffer[bufferSize] = "a";
				uint8_t smallBuffer[] = {"hallo"};

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
				BOOST_CHECK_MESSAGE(result == bufferSize, strerror(errno));

				clearAnswer();

				hbm::communication::dataBlock_t smallDatablocks[4];
				size_t bufferCount = sizeof(smallDatablocks)/sizeof(hbm::communication::dataBlock_t);

				for (size_t blockIndex = 0; blockIndex<bufferCount; ++blockIndex) {
					smallDatablocks[blockIndex].pData = smallBuffer;
					smallDatablocks[blockIndex].size = sizeof(smallBuffer);
				}


				result = client.sendBlocks(smallDatablocks, bufferCount, true);
				BOOST_CHECK_MESSAGE((size_t)result == sizeof(smallBuffer)*bufferCount, strerror(errno));
				result = client.sendBlocks(smallDatablocks, bufferCount, false);
				BOOST_CHECK_MESSAGE((size_t)result == sizeof(smallBuffer)*bufferCount, strerror(errno));

				client.disconnect();

				stop();
			}


#define BLOCKCOUNT 10

			BOOST_AUTO_TEST_CASE(writev_test_array)
			{
				int result;
				static const size_t bufferSize = 100000;
				static const size_t blockSize = bufferSize/BLOCKCOUNT;
				char buffer[bufferSize] = "a";

				hbm::communication::dataBlock_t dataBlockArray[BLOCKCOUNT];
				hbm::communication::dataBlocks_t dataBlocks;

				for(unsigned int i=0; i<BLOCKCOUNT; ++i) {
					dataBlockArray[i].pData = &buffer[i*blockSize];
					dataBlockArray[i].size = blockSize;
				}

				start();

				hbm::communication::SocketNonblocking client(m_eventloop);
				result = client.connect("127.0.0.1", std::to_string(PORT));
				BOOST_CHECK_MESSAGE(result == 0, strerror(errno));

				client.setDataCb(std::bind(&serverFixture::clientReceive, this, std::placeholders::_1));

				clearAnswer();
				result = client.sendBlocks(dataBlockArray, 10);
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				client.disconnect();

				BOOST_CHECK_MESSAGE(result == bufferSize, strerror(errno));


				stop();
			}

			BOOST_AUTO_TEST_CASE(receive_wouldblock)
			{
				char buffer[1000];
				hbm::communication::SocketNonblocking client(m_eventloop);
				client.connect("127.0.0.1", std::to_string(PORT));
				
				ssize_t result = client.receive(buffer, sizeof(buffer));
				BOOST_CHECK_EQUAL(result, -1);
				BOOST_CHECK_EQUAL(errno, EWOULDBLOCK);
			}
			
#ifndef _WIN32			
			BOOST_AUTO_TEST_CASE(send_wouldblock)
			{
				size_t bytesSend = 0;
				unsigned int sendBufferSize = 1000;
				socklen_t len;
				char data[1000000] = { 0 };
				hbm::communication::SocketNonblocking client(m_eventloop);
				client.connect("127.0.0.1", std::to_string(PORT));
				
				unsigned int notifiyCount = 0;
				client.setOutDataCb(std::bind(&serverFixture::clientNotify, this, std::ref(notifiyCount)));
				// callback function gets called once to check for pending work...
				BOOST_CHECK_EQUAL(notifiyCount, 1);
				
				int result = setsockopt(client.getEvent(), SOL_SOCKET, SO_SNDBUF, &sendBufferSize, sizeof(sendBufferSize));
				BOOST_CHECK_EQUAL(result, 0);
				len = sizeof(sendBufferSize);
				getsockopt(client.getEvent(), SOL_SOCKET, SO_SNDBUF, &sendBufferSize, &len);
				
				ssize_t sendResult;
				// send until send buffer is full
				do {
					sendResult = client.send(data, sizeof(data), false);
					bytesSend += sendResult;
				} while (sendResult>0);
				BOOST_CHECK_GE(bytesSend, sendBufferSize);
				BOOST_CHECK_EQUAL(errno, EWOULDBLOCK);

				// server is not receiving, hence socket won't become writable and callback function is not being called.
				BOOST_CHECK_EQUAL(notifiyCount, 1);
				start();
				// server is receiving. As a result socket becomes wriable again. Callback function is called!
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				
				BOOST_CHECK_EQUAL(notifiyCount, 2);
				
				// We should be able to send again
				sendResult = client.send(data, sizeof(data), false);
				BOOST_CHECK_GT(bytesSend, 0);
				stop();
			}
#endif


			BOOST_AUTO_TEST_SUITE_END()
		}
	}
}
