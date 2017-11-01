// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#ifndef __HBM__COMMUNICATION_TCPACCEPTOR_H
#define __HBM__COMMUNICATION_TCPACCEPTOR_H

#include <memory>
#include <string>

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
#undef max
#undef min
#endif



#include "hbm/communication/socketnonblocking.h"
#include "hbm/sys/eventloop.h"

namespace hbm {
	namespace communication {
		class TcpServer {
		public:
			/// deliveres the worker socket for an accepted client
			typedef std::function < void (clientSocket_t) > Cb_t;

			/// @param eventLoop Event loop the object will be registered in 
			TcpServer(sys::EventLoop &eventLoop);
			virtual ~TcpServer();

			/// @param port TCP port to listen to
			/// @param backlog Maximum length of the queue of pending connections
			/// @param acceptCb called when accepting a new tcp client
			int start(uint16_t port, int backlog, Cb_t acceptCb);

			/// Remove this object from the event loop and close the server socket
			void stop();

		private:

			/// should not be copied
			TcpServer(const TcpServer& op);

			/// should not be assigned
			TcpServer& operator= (const TcpServer& op);

			/// called by eventloop
			/// accepts a new connection creates new worker socket anf calls acceptCb
			int process();

#ifdef _WIN32
			/// accepts a new connecting client.
			/// \return On success, the worker socket for the new connected client is returned. Empty worker socket on error
			clientSocket_t acceptClient();

			int prepareAccept();
#endif

			sys::event m_listeningEvent;
#ifdef _WIN32
			SOCKET m_acceptSocket;

			LPFN_ACCEPTEX m_acceptEx;
			char m_acceptBuffer[1024];
			DWORD m_acceptSize;
#endif
			sys::EventLoop& m_eventLoop;
			Cb_t m_acceptCb;
		};
	}
}
#endif
