// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#ifndef __HBM__COMMUNICATION_SOCKETTEST_H
#define __HBM__COMMUNICATION_SOCKETTEST_H

#include <thread>
#include <set>

#include "hbm/communication/socketnonblocking.h"
#include "hbm/communication/tcpserver.h"

#include "hbm/sys/eventloop.h"


namespace hbm {
	namespace communication {
		namespace test {

			class serverFixture {
			public:
				int clientReceive(SocketNonblocking &socket);
				int clientReceiveSingleBytes(SocketNonblocking& socket);
			protected:
				serverFixture();
				virtual ~serverFixture();
				void acceptCb(workerSocket_t worker);
				int serverEcho();
				void clearAnswer()
				{
					m_answer.clear();
				}

				std::string getAnswer() const
				{
					return m_answer;
				}

			private:
				sys::EventLoop m_eventloop;
				std::thread m_serverWorker;
				workerSocket_t m_worker;
				TcpServer m_server;

				std::string m_answer;
			};
		}
	}
}

#endif
