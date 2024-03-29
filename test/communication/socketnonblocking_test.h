// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#ifndef __HBM__COMMUNICATION_SOCKETTEST_H
#define __HBM__COMMUNICATION_SOCKETTEST_H

#include <future>
#include <map>

#include "hbm/communication/socketnonblocking.h"
#include "hbm/communication/tcpserver.h"

#include "hbm/sys/eventloop.h"


namespace hbm {
	namespace communication {
		namespace test {

			class serverFixture {
			public:
				int clientReceive(SocketNonblocking &socket);
				int clientReceiveTarget(SocketNonblocking& socket, std::string& target);
				int clientReceiveSingleBytes(SocketNonblocking& socket);

				int clientNotify(unsigned int& count);

				void start();
				void stop();
				size_t getClientCount() const;
				sys::EventLoop m_eventloop;
			protected:
				serverFixture();
				virtual ~serverFixture();
				void acceptCb(clientSocket_t worker);
				ssize_t serverEcho(int clientId);
				void clearAnswer()
				{
					m_answer.clear();
				}

				std::string getAnswer() const
				{
					return m_answer;
				}

			private:
				std::future < int > m_serverWorker;

				std::map < int, clientSocket_t> m_workers;
				TcpServer m_server;

				std::string m_answer;
			};
		}
	}
}

#endif
