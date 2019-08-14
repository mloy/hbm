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
			/// callback method type executed by event loop on arrival of data
			typedef std::function < ssize_t (MulticastServer& mcs) > DataHandler_t;

			/// \param netadapterList Need to know about the interfaces available
			/// \param eventLoop Callback methods are executed in this context
			MulticastServer(NetadapterList& netadapterList, sys::EventLoop &eventLoop);

			virtual ~MulticastServer();

			/// Add interface to multicast group. Interface will receive from this multicast group.
			/// \return 1 if interface was added succesfully, 0 interface already member of multicast group, -1 error
			int addInterface(const std::string& interfaceAddress);

			/// Add interface to multicast group. Interface will receive from this multicast group.
			/// \return 1 if interface was added succesfully, 0 interface already member of multicast group, -1 error
			int addInterface(unsigned int interfaceIndex);

			/// all interfaces known to the internal netadapter list are added as receiving interfaces.
			void addAllInterfaces();

			/// Drop interface from multicast group. Interface will no longer receive from this multicast group.
			/// \return 1 if interface was dropped succesfully, 0 interface not member of multicast group, -1 error
			int dropInterface(const std::string& interfaceAddress);

			/// Drop interface from multicast group. Interface will no longer receive from this multicast group.
			/// \return 1 if interface was dropped succesfully, 0 interface not member of multicast group, -1 error
			int dropInterface(unsigned int interfaceIndex);

			/// all interfaces known to the internal netadapter list are dropped as receiving interfaces.
			void dropAllInterfaces();

			/// @param multicastGroup multicast group to register to
			/// @param port udp/ip port to use
			/// @param dataHandler set to empty(DataHandler_t()) if object is used as sender only.
			int start(const std::string& multicastGroup, unsigned int port, DataHandler_t dataHandler);

			/// unregister from eventloop, close sender and receiver
			void stop();

			/// tells whether send messages are to be received by multicast receiver on the same machine.
			/// Especially the very same process will receive what it send!
			/// off by default
			int setMulticastLoop(bool value);

			/// Send over all interfaces
			/// @param data Data to send
			/// @param ttl number of hops which means number of routers to pass
			int send(const std::string& data, unsigned int ttl=1) const;

			/// Send over all interfaces
			/// @param pData Data to send
			/// @param length Amount of data to send
			/// @param ttl number of hops which means number of routers to pass
			int send(const void *pData, size_t length, unsigned int ttl=1) const;

			/// send over specific interface
			/// @param adapter interface to use
			/// @param data to send
			/// @param ttl number of hops which means number of routers to pass
			int sendOverInterface(const Netadapter& adapter, const std::string& data, unsigned int ttl=1) const;
			/// @param adapter interface to use
			/// @param pData buffer to send
			/// @param length size of buffer to send
			/// @param ttl number of hops which means number of routers to pass
			int sendOverInterface(const Netadapter &adapter, const void* pData, size_t length, unsigned int ttl=1) const;
			/// @param interfaceIndex interface to use
			/// @param data to send
			/// @param ttl number of hops which means number of routers to pass
			int sendOverInterface(unsigned int interfaceIndex, const std::string& data, unsigned int ttl=1) const;
			/// @param interfaceIndex interface to use
			/// @param pData buffer to send
			/// @param length size of buffer to send
			/// @param ttl number of hops which means number of routers to pass
			int sendOverInterface(unsigned int interfaceIndex, const void* pData, size_t length, unsigned int ttl=1) const;


			/// send over specific interface.
			/// @param interfaceIp IP address of the interface to use
			/// @param data Data to send
			/// @param ttl number of maximum hops. n Means crossing n-1 router. Of course the routers are to be configured to allow multicast messages to be routed!
			int sendOverInterfaceByAddress(const std::string& interfaceIp, const std::string& data, unsigned int ttl=1) const;

			/// send over specific interface.
			/// @param interfaceIp IP address of the interface to use
			/// @param pData Data to send
			/// @param length Amount of data to send
			/// @param ttl number of maximum hops. n Means crossing n-1 router. Of course the routers are to be configured to allow multicast messages to be routed!
			int sendOverInterfaceByAddress(const std::string& interfaceIp, const void* pData, size_t length, unsigned int ttl=1) const;

			/// send over specific interface.
			int sendOverInterfaceByIndex(unsigned int interfaceIndex, const std::string& data, unsigned int ttl=1) const;

			/// send over specific interface.
			int sendOverInterfaceByIndex(unsigned int interfaceIndex, const void* pData, size_t length, unsigned int ttl=1) const;

			/// \param msgbuf Buffer for data to receive
			/// \param len Size of buffer for data to receive
			/// \param adapter The interface received from
			/// @param ttl number of hops which means number of routers to pass
			ssize_t receiveTelegram(void* msgbuf, size_t len, Netadapter& adapter, int &ttl);

			/// \param msgbuf Buffer for data to receive
			/// \param len Size of buffer for data to receive
			/// \param adapterName The interface received from
			/// @param ttl number of hops which means number of routers to pass
			ssize_t receiveTelegram(void* msgbuf, size_t len, std::string& adapterName, int& ttl);

			/// @param msgbuf Buffer for received data
			/// @param len Size of the buffer
			/// @param adapterIndex Network adapter to receive from
			/// @param[out] ttl ttl in the ip header (the value set by the last sender(router))
			ssize_t receiveTelegram(void* msgbuf, size_t len, unsigned int& adapterIndex, int &ttl);
		private:

			MulticastServer(const MulticastServer&);
			MulticastServer& operator=(const MulticastServer&);

			int setupSendSocket();

			/// setup the receiver socket and the address structure.
			int setupReceiveSocket();

			int dropOrAddInterface(const std::string& interfaceAddress, bool add);
			int dropOrAddInterface(unsigned int interfaceIndex, bool add);

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
