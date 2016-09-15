// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#ifndef _MULTICASTSERVER_H
#define _MULTICASTSERVER_H


#include <string>
#include <chrono>



#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
//#undef max
//#undef min
#else
#include <arpa/inet.h>
#endif

#include "netadapter.h"
#include "netadapterlist.h"

#include "hbm/sys/eventloop.h"

#ifdef _WIN32
#ifndef ssize_t
typedef int ssize_t;
#endif
#else
#ifndef SOCKET
typedef int SOCKET;
#endif
#endif

namespace hbm {
	namespace communication {
		/// The maximum datagram size supported by UDP
		const unsigned int MAX_DATAGRAM_SIZE = 65536;

		class Netadapter;

		/// for receiving/sending UDP packets from/to multicast groups
		class MulticastServer
		{
		public:
			typedef std::function < ssize_t (MulticastServer& mcs) > DataHandler_t;

			/// @param address the multicast group
			MulticastServer(NetadapterList& netadapterList, sys::EventLoop &eventLoop);

			virtual ~MulticastServer();

			/// \return 1 if interface was added succesfully, 0 interface already member of multicast group, -1 error
			int addInterface(const std::string& interfaceAddress);

			/// all interfaces known to the internal netadapter list are added as receiving interfaces.
			void addAllInterfaces();

			/// \return 1 if interface was dropped succesfully, 0 interface not member of multicast group, -1 error
			int dropInterface(const std::string& interfaceAddress);

			/// all interfaces known to the internal netadapter list are dropped as receiving interfaces.
			void dropAllInterfaces();

			/// @param dataHandler set to empty(DataHandler_t()) if object is used as sender only.
			int start(const std::string& multicastGroup, unsigned int port, DataHandler_t dataHandler);

			void stop();

			/// tells whether send messages are to be received by multicast receiver on the same machine.
			/// Especially the very same process will receive what it send!
			/// off by default
			int setMulticastLoop(bool value);

			/// Send over all interfaces
			int send(const std::string& data, unsigned int ttl=1) const;

			int send(const void *pData, size_t length, unsigned int ttl=1) const;

			/// send over specific interface
			int sendOverInterface(const Netadapter& adapter, const std::string& data, unsigned int ttl=1) const;
			int sendOverInterface(const Netadapter &adapter, const void* pData, size_t length, unsigned int ttl=1) const;

			int sendOverInterface(int interfaceIndex, const std::string& data, unsigned int ttl=1) const;
			int sendOverInterface(int interfaceIndex, const void* pData, size_t length, unsigned int ttl=1) const;


			/// send over specific interface.
			/// @param interfaceIp IP address of the interface to use
			int sendOverInterfaceByAddress(const std::string& interfaceIp, const std::string& data, unsigned int ttl=1) const;
			int sendOverInterfaceByAddress(const std::string& interfaceIp, const void* pData, size_t length, unsigned int ttl=1) const;

			ssize_t receiveTelegram(void* msgbuf, size_t len, Netadapter& adapter, int &ttl);
			ssize_t receiveTelegram(void* msgbuf, size_t len, std::string& adapterName, int& ttl);

			/// @param[out] ttl ttl in the ip header (the value set by the last sender(router))
			ssize_t receiveTelegram(void* msgbuf, size_t len, unsigned int& adapterIndex, int &ttl);
		private:

			MulticastServer(const MulticastServer&);
			MulticastServer& operator=(const MulticastServer&);

			int setupSendSocket();

			/// setup the receiver socket and the address structure.
			int setupReceiveSocket();

			int dropOrAddInterface(const std::string& interfaceAddress, bool add);

			/// called by eventloop
			int process();

#ifdef _WIN32
			int orderNextMessage();
#endif

			std::string m_mutlicastgroup;

			unsigned int m_port;


			sys::event m_receiveEvent;
			sys::event m_sendEvent;
	#ifdef _WIN32
			LPFN_WSARECVMSG m_wsaRecvMsg;

			WSAMSG m_msg;
			WSABUF m_iov;
			uint8_t m_recvBuffer[MAX_DATAGRAM_SIZE];
			char m_recvControlBuffer[100];
	#endif

			struct sockaddr_in m_receiveAddr;

			const NetadapterList& m_netadapterList;

			sys::EventLoop& m_eventLoop;
			DataHandler_t m_dataHandler;
		};
	}
}
#endif
