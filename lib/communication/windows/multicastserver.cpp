// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#include <vector>
#include <cstring>
#include <cstdio>
#include <iostream>

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>

#define syslog fprintf
#define sprintf sprintf_s
#define LOG_DEBUG stdout
#define LOG_INFO stdout
#define LOG_ERR stderr

#include "hbm/exception/exception.hpp"
#include "hbm/sys/eventloop.h"

#include "hbm/communication/multicastserver.h"


static WSABUF signalBuffer = { 0, NULL };

namespace hbm {
	namespace communication {
		MulticastServer::MulticastServer(NetadapterList& netadapterList, sys::EventLoop &eventLoop)
			: m_mutlicastgroup()
			, m_port()
			, m_receiveAddr()
			, m_netadapterList(netadapterList)
			, m_eventLoop(eventLoop)
			, m_dataHandler()
		{
			WORD RequestedSockVersion = MAKEWORD(2, 2);
			WSADATA wsaData;
			WSAStartup(RequestedSockVersion, &wsaData);

			m_receiveEvent.completionPort = m_eventLoop.getCompletionPort();
			m_receiveEvent.overlapped.hEvent = WSACreateEvent();
			m_receiveEvent.fileHandle = INVALID_HANDLE_VALUE;
			m_sendEvent.fileHandle = INVALID_HANDLE_VALUE;
		}

		MulticastServer::~MulticastServer()
		{
			stop();
			WSACloseEvent(m_receiveEvent.overlapped.hEvent);
		}

		int MulticastServer::setupReceiveSocket()
		{
			{
				struct addrinfo hints;
				struct addrinfo* pResult = NULL;
				char portString[8];

				memset(&hints, 0, sizeof(hints));

				sprintf(portString, "%u", m_port);
				hints.ai_family   = PF_UNSPEC;
				hints.ai_socktype = SOCK_DGRAM;

				if ( getaddrinfo(m_mutlicastgroup.c_str(), portString, &hints, &pResult) != 0 ) {
					::syslog(LOG_ERR, "Not a valid multicast IP address!");
					return -1;
				}

				m_receiveEvent.fileHandle = reinterpret_cast < HANDLE > (::WSASocket(pResult->ai_family, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED));
				if (m_receiveEvent.fileHandle < 0) {
					m_receiveEvent.fileHandle = INVALID_HANDLE_VALUE;
				}

				memset(&m_receiveAddr, 0, sizeof(m_receiveAddr));
				m_receiveAddr.sin_family = pResult->ai_family;
				m_receiveAddr.sin_addr.s_addr = htonl(INADDR_ANY);
				m_receiveAddr.sin_port = htons(m_port);

				freeaddrinfo(pResult);
			}
						


			if (m_receiveEvent.fileHandle < 0) {
				::syslog(LOG_ERR, "Could not create receiving socket!");
				return -1;
			}

			// switch to non blocking
			u_long value = 1;
			::ioctlsocket(reinterpret_cast < SOCKET > (m_receiveEvent.fileHandle), FIONBIO, &value);



			// sufficient buffer for several messages
			int RcvBufSize = 128000;
			if (setsockopt(reinterpret_cast < SOCKET > (m_receiveEvent.fileHandle), SOL_SOCKET, SO_RCVBUF, reinterpret_cast < char* >(&RcvBufSize), sizeof(RcvBufSize)) < 0) {
				return -1;
			}

			uint32_t yes = 1;
			// allow multiple sockets to use the same PORT number
			if (setsockopt(reinterpret_cast < SOCKET > (m_receiveEvent.fileHandle), SOL_SOCKET, SO_REUSEADDR, reinterpret_cast < char* >(&yes), sizeof(yes)) < 0) {
				::syslog(LOG_ERR, "Could not set SO_REUSEADDR!");
				return -1;
			}

			// We do want to know the interface, we received on
			if (setsockopt(reinterpret_cast < SOCKET > (m_receiveEvent.fileHandle), IPPROTO_IP, IP_PKTINFO, reinterpret_cast < char* >(&yes), sizeof(yes)) != 0) {
				::syslog(LOG_ERR, "Could not set IP_PKTINFO!");
				return -1;
			}

			//if (setsockopt(m_ReceiveSocket, IPPROTO_IP, IP_RECVTTL, reinterpret_cast < char* >(&yes), sizeof(yes)) != 0) {
			//	::syslog(LOG_ERR, "Could not set IP_RECVTTL!");
			//	return -1;
			//}

			if (bind(reinterpret_cast < SOCKET > (m_receiveEvent.fileHandle), (struct sockaddr*)&m_receiveAddr, sizeof(m_receiveAddr)) < 0) {
				::syslog(LOG_ERR, "Could not bind socket!");
				return -1;
			}

			DWORD winLen;

			GUID InBuffer[] = WSAID_WSARECVMSG;
			int status = WSAIoctl(reinterpret_cast < SOCKET > (m_receiveEvent.fileHandle), SIO_GET_EXTENSION_FUNCTION_POINTER, &InBuffer, sizeof(InBuffer), &m_wsaRecvMsg, sizeof(m_wsaRecvMsg), &winLen, NULL, NULL);
			if (status != 0) {
				::syslog(LOG_ERR, "could not initialize WSARecvMsg");
				return -1;
			}

			return 0;
		}


		int MulticastServer::setupSendSocket()
		{
			m_sendEvent.fileHandle = reinterpret_cast < HANDLE > (WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED));
			if (m_sendEvent.fileHandle == INVALID_HANDLE_VALUE) {
				::syslog(LOG_ERR, "Could not create socket!");
				return -1;
			}


			// we do not want to receive the stuff we where sending
			setMulticastLoop(false);
			return 0;
		}

		int MulticastServer::setMulticastLoop(bool value)
		{
			unsigned char param = 0;
			if (value) {
				param = 1;
			}

			if (setsockopt(reinterpret_cast < SOCKET > (m_sendEvent.fileHandle), IPPROTO_IP, IP_MULTICAST_LOOP, reinterpret_cast <char* > (&param), sizeof(param))) {
				::syslog(LOG_ERR, "Error setsockopt IP_MULTICAST_LOOP!");
				return -1;
			}
			return 0;
		}


		int MulticastServer::dropInterface(const std::string& interfaceAddress)
		{
			return dropOrAddInterface(interfaceAddress, false);
		}

		int MulticastServer::dropInterface(int interfaceIndex)
		{
			return dropOrAddInterface(interfaceIndex, false);
		}

		int MulticastServer::addInterface(const std::string& interfaceAddress)
		{
			return dropOrAddInterface(interfaceAddress, true);
		}

		int MulticastServer::addInterface(int interfaceIndex)
		{
			return dropOrAddInterface(interfaceIndex, true);
		}


		void MulticastServer::addAllInterfaces()
		{
			NetadapterList::Adapters adapters = m_netadapterList.get();
			for (NetadapterList::Adapters::const_iterator iter = adapters.begin(); iter != adapters.end(); ++iter) {
				const communication::Netadapter& adapter = iter->second;
				dropOrAddInterface(adapter.getIndex(), true);
			}
		}

		void MulticastServer::dropAllInterfaces()
		{
			NetadapterList::Adapters adapters = m_netadapterList.get();
			for (NetadapterList::Adapters::const_iterator iter = adapters.begin(); iter != adapters.end(); ++iter) {
				const communication::Netadapter& adapter = iter->second;
				dropOrAddInterface(adapter.getIndex(), false);
			}
		}


		int MulticastServer::dropOrAddInterface(const std::string& interfaceAddress, bool add)
		{
			int retVal = 0;
			struct addrinfo hints;
			struct addrinfo* pResult = NULL;
			char portString[8];

			struct ip_mreq im;

			memset(&hints, 0, sizeof(hints));

			sprintf(portString, "%u", m_port);
			hints.ai_family   = PF_UNSPEC;
			hints.ai_socktype = SOCK_DGRAM;

			if ( getaddrinfo(m_mutlicastgroup.c_str(), portString, &hints, &pResult) != 0 ) {
				::syslog(LOG_ERR, "Not a valid multicast IP address (%s)!", m_mutlicastgroup.c_str());
				return -1;
			}

			memset(&im, 0, sizeof(im));
			memcpy(&im.imr_multiaddr, &(reinterpret_cast < struct sockaddr_in* > (pResult->ai_addr))->sin_addr, sizeof(im.imr_multiaddr.s_addr));

			freeaddrinfo(pResult);

			im.imr_interface.s_addr = inet_addr(interfaceAddress.c_str());
			if (im.imr_interface.s_addr == INADDR_NONE) {
				::syslog(LOG_ERR, "not a valid IP address for IP_ADD_MEMBERSHIP!");
				return -1;
			}

			int optionName;
			if(add) {
				optionName = IP_ADD_MEMBERSHIP;
			} else {
				optionName = IP_DROP_MEMBERSHIP;
			}


			retVal = setsockopt(reinterpret_cast < SOCKET > (m_receiveEvent.fileHandle), IPPROTO_IP, optionName, reinterpret_cast < char* >(&im), sizeof(im));
			if (add) {
				if (retVal!=0) {
					if(errno==WSAEADDRINUSE) {
						// ignore already added
						return 0;
					}
					return -1;
				}
				return 1;
			} else {
				if(retVal!=0) {
					if (errno == WSAEADDRNOTAVAIL) {
						// ignore already dropped
						return 0;
					}
					return -1;
				}
				return 1;
			}
		}

		int MulticastServer::dropOrAddInterface(int interfaceIndex, bool add)
		{
			int retVal = 0;
			struct addrinfo hints;
			struct addrinfo* pResult = NULL;
			char portString[8];

			struct ip_mreq im;

			memset(&hints, 0, sizeof(hints));

			sprintf(portString, "%u", m_port);
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_DGRAM;

			if (getaddrinfo(m_mutlicastgroup.c_str(), portString, &hints, &pResult) != 0) {
				::syslog(LOG_ERR, "Not a valid multicast IP address (%s)!", m_mutlicastgroup.c_str());
				return -1;
			}

			if (pResult->ai_addr->sa_family != AF_INET) {
				::syslog(LOG_ERR, "Only IPv4 multicast IP address currently supported (%s)!", m_mutlicastgroup.c_str());
				return -1;
			}

			struct sockaddr_in address;
			std::memcpy(&address, pResult->ai_addr, sizeof(address));

			memset(&im, 0, sizeof(im));
			im.imr_multiaddr = address.sin_addr;
			im.imr_interface.S_un.S_addr = htonl(interfaceIndex);

			freeaddrinfo(pResult);

			int optionName;
			if (add) {
				optionName = IP_ADD_MEMBERSHIP;
			}
			else {
				optionName = IP_DROP_MEMBERSHIP;
			}

			retVal = setsockopt(reinterpret_cast < SOCKET > (m_receiveEvent.fileHandle), IPPROTO_IP, optionName, reinterpret_cast < char* >(&im), sizeof(im));
			if (add) {
				if (retVal != 0) {
					if (errno == EADDRINUSE) {
						// ignore already added
						return 0;
					}
					syslog(LOG_ERR, "interface address %d could not be added to multicastgroup %s '%s'", interfaceIndex, m_mutlicastgroup.c_str(), strerror(errno));
					return -1;
				}
				return 1;
			}
			else {
				if (retVal != 0) {
					if (errno == EADDRNOTAVAIL) {
						// ignore already dropped
						return 0;
					}
					return -1;
				}
				return 1;
			}
		}

		int MulticastServer::process()
		{
			if (m_dataHandler) {
				return m_dataHandler(*this);
			} else {
				return -1;
			}
		}

		ssize_t MulticastServer::receiveTelegram(void* msgbuf, size_t len, Netadapter& adapter, int& ttl)
		{
			unsigned int interfaceIndex = 0;
			ssize_t nbytes = receiveTelegram(msgbuf, len, interfaceIndex, ttl);
			if(nbytes>0) {
				try {
					adapter = m_netadapterList.getAdapterByInterfaceIndex(interfaceIndex);
				} catch( const hbm::exception::exception&) {
					::syslog(LOG_ERR, "%s no interface with index %d!", __FUNCTION__, interfaceIndex);
					nbytes = -1;
				}
			}
			return nbytes;
		}

		ssize_t MulticastServer::receiveTelegram(void* msgbuf, size_t len, std::string& adapterName, int& ttl)
		{
			unsigned int interfaceIndex = 0;
			ssize_t nbytes = receiveTelegram(msgbuf, len, interfaceIndex, ttl);
			if(nbytes>0) {
				try {
					adapterName = m_netadapterList.getAdapterByInterfaceIndex(interfaceIndex).getName();
				} catch( const hbm::exception::exception&) {
					::syslog(LOG_ERR, "%s no interface with index %d!", __FUNCTION__, interfaceIndex);
					nbytes = -1;
				}
			}
			return nbytes;
		}

		ssize_t MulticastServer::receiveTelegram(void* msgBuf, size_t msgBufSize, unsigned int& adapterIndex, int& ttl)
		{
		        ttl = 1;
			if (m_receiveEvent.overlapped.InternalHigh == 0) {
				return -1;
			}

			size_t size = m_receiveEvent.overlapped.InternalHigh;

			if (size > msgBufSize) {
				size = msgBufSize;
			}
			memcpy(msgBuf, m_recvBuffer, size);
			m_receiveEvent.overlapped.InternalHigh = 0;

			for (WSACMSGHDR* pcmsghdr = WSA_CMSG_FIRSTHDR(&m_msg); pcmsghdr != NULL; pcmsghdr = WSA_CMSG_NXTHDR(&m_msg, pcmsghdr))
			{
				if (pcmsghdr->cmsg_type == IP_PKTINFO) {
					struct in_pktinfo* ppktinfo;
					ppktinfo = reinterpret_cast <struct in_pktinfo*> (WSA_CMSG_DATA(pcmsghdr));
					adapterIndex = ppktinfo->ipi_ifindex;
				}
				else if (pcmsghdr->cmsg_type == IP_TTL) {
					int* pTtl;
					// returns the ttl from the received ip header (the value set by the last sender(router))
					pTtl = reinterpret_cast <int*> (WSA_CMSG_DATA(pcmsghdr));
					ttl = *pTtl;
				}
			}

			orderNextMessage();

			return size;
		}

		int MulticastServer::send(const void* pData, size_t length, unsigned int ttl) const
		{
			int retVal = 0;
			int retValIntern;

			NetadapterList::Adapters adapters = m_netadapterList.get();

			for (NetadapterList::Adapters::const_iterator iter = adapters.begin(); iter != adapters.end(); ++iter) {
				const Netadapter& adapter = iter->second;

				retValIntern = sendOverInterface(adapter, pData, length, ttl);
				if (retValIntern != 0) {
					retVal = retValIntern;
				}
			}

			return retVal;
		}

		int MulticastServer::send(const std::string& data, unsigned int ttl) const
		{
			return send(data.c_str(), data.length(), ttl);
		}

		int MulticastServer::sendOverInterface(const Netadapter& adapter, const std::string& data, unsigned int ttl) const
		{
			if (data.empty()) {
				return 0;
			}

			int retVal = 0;

			const communication::AddressesWithNetmask addressesWithNetmask = adapter.getIpv4Addresses();
			if(addressesWithNetmask.empty()) {
				return communication::ERR_NO_SUCCESS;
			} else {
				retVal = sendOverInterfaceByAddress(addressesWithNetmask.front().address, data, ttl);
			}

			return retVal;
		}

		int MulticastServer::sendOverInterface(int interfaceIndex, const std::string& data, unsigned int ttl) const
		{
			if (data.empty()) {
				return 0;
			}

			// IPV6_MULTICAST_IF does not work. It is not possible to send via a desired interface index. We have to select the interface by IP address using IP_MULTICAST_IF
			int retVal;
			try {
				communication::Netadapter adapter = m_netadapterList.getAdapterByInterfaceIndex(interfaceIndex);
				retVal = sendOverInterface(adapter, data, ttl);
			} catch( const hbm::exception::exception&) {
				retVal = communication::ERR_INVALIDADAPTER;
			}
			return retVal;
		}

		int MulticastServer::sendOverInterface(int interfaceIndex, const void* pData, size_t length, unsigned int ttl) const
		{
			if (pData==NULL) {
				return 0;
			}

			// IPV6_MULTICAST_IF does not work. It is not possible to send via a desired interface index. We have to select the interface by IP address using IP_MULTICAST_IF

			int retVal;
			try {
				Netadapter adapter = m_netadapterList.getAdapterByInterfaceIndex(interfaceIndex);
				retVal = sendOverInterface(adapter, pData, length, ttl);
			} catch( const hbm::exception::exception&) {
				retVal = ERR_INVALIDADAPTER;
			}
			return retVal;
		}

		int MulticastServer::sendOverInterface(const Netadapter& adapter, const void* pData, size_t length, unsigned int ttl) const
		{
			int retVal = ERR_SUCCESS;
			if(length>0) {
				if (pData==NULL) {
					retVal = ERR_NO_SUCCESS;
				} else {
					const communication::AddressesWithNetmask addressesWithNetmask = adapter.getIpv4Addresses();
					if(addressesWithNetmask.empty()==false) {
						retVal = sendOverInterfaceByAddress(addressesWithNetmask.front().address, pData, length, ttl);
					} else {
						retVal = ERR_INVALIDADAPTER;
						::syslog(LOG_ERR, "%s interface %s does not have an ipv4 address!", __FUNCTION__, adapter.getName().c_str());
					}
				}
			}

			return retVal;
		}

		int MulticastServer::sendOverInterfaceByAddress(const std::string& interfaceIp, const std::string& data, unsigned int ttl) const
		{
			if(data.empty()) {
				return ERR_SUCCESS;
			}

			return sendOverInterfaceByAddress(interfaceIp, data.c_str(), data.length(), ttl);
		}

		int MulticastServer::sendOverInterfaceByAddress(const std::string& interfaceIp, const void* pData, size_t length, unsigned int ttl) const
		{
			if (pData==NULL) {
				if(length>0) {
					return ERR_NO_SUCCESS;
				} else {
					return ERR_SUCCESS;
				}
			}

			struct in_addr ifAddr;
			memset(&ifAddr, 0, sizeof(ifAddr));


			ifAddr.s_addr = inet_addr(interfaceIp.c_str());

			if (ifAddr.s_addr == INADDR_NONE) {
				::syslog(LOG_ERR, "%s is not a valid interface IP address!", interfaceIp.c_str());
				return ERR_INVALIDIPADDRESS;
			}

			if (setsockopt(reinterpret_cast < SOCKET > (m_sendEvent.fileHandle), IPPROTO_IP, IP_MULTICAST_IF, reinterpret_cast < char* >(&ifAddr), sizeof(ifAddr))) {
				::syslog(LOG_ERR, "Error setsockopt IP_MULTICAST_IF for interface %s!", interfaceIp.c_str());
				return ERR_INVALIDADAPTER;
			};

			if (setsockopt(reinterpret_cast < SOCKET > (m_sendEvent.fileHandle), IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast < char* >(&ttl), sizeof(ttl))) {
				::syslog(LOG_ERR, "%s: Error setsockopt IPV6_MULTICAST_HOPS to %u!", interfaceIp.c_str(), ttl);
				return ERR_NO_SUCCESS;
			}

			struct sockaddr_in sendAddr;
			memset(&sendAddr, 0, sizeof(sendAddr));
			sendAddr.sin_family = AF_INET;
			sendAddr.sin_port = htons(m_port);
			sendAddr.sin_addr.s_addr = inet_addr(m_mutlicastgroup.c_str());

			if (sendAddr.sin_addr.s_addr == INADDR_NONE) {
				::syslog(LOG_ERR, "Not a valid multicast IP address!");
				return ERR_NO_SUCCESS;
			}

			ssize_t nbytes = sendto(reinterpret_cast < SOCKET > (m_sendEvent.fileHandle), reinterpret_cast < const char* > (pData), static_cast < int > (length), 0, reinterpret_cast < struct sockaddr* >(&sendAddr), sizeof(sendAddr));

			if (static_cast < size_t >(nbytes) != length) {
				::syslog(LOG_ERR, "error sending message over interface %s!", interfaceIp.c_str());
				return ERR_NO_SUCCESS;
			}
			return ERR_SUCCESS;
		}

		int MulticastServer::sendOverInterfaceByIndex(int interfaceIndex, const std::string& data, unsigned int ttl) const
		{
			if (data.empty()) {
				return ERR_SUCCESS;
			}

			return sendOverInterfaceByIndex(interfaceIndex, data.c_str(), data.length(), ttl);
		}

		int MulticastServer::sendOverInterfaceByIndex(int interfaceIndex, const void* pData, size_t length, unsigned int ttl) const
		{
			if (pData == NULL) {
				if (length>0) {
					return ERR_NO_SUCCESS;
				}
				else {
					return ERR_SUCCESS;
				}
			}

			struct ip_mreq req;
			memset(&req, 0, sizeof(req));
			req.imr_interface.S_un.S_addr = interfaceIndex;
			if (setsockopt(reinterpret_cast < SOCKET > (m_sendEvent.fileHandle), IPPROTO_IP, IP_MULTICAST_IF, reinterpret_cast < char* >(&req), sizeof(req))) {
				::syslog(LOG_ERR, "Error setsockopt IP_MULTICAST_IF for interface %d!", interfaceIndex);
				return ERR_INVALIDADAPTER;
			}

			if (setsockopt(reinterpret_cast < SOCKET > (m_sendEvent.fileHandle), IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast < char* >(&ttl), sizeof(ttl))) {
				::syslog(LOG_ERR, "Error setsockopt IPV6_MULTICAST_HOPS for interface %d to %u!", interfaceIndex, ttl);
				return ERR_NO_SUCCESS;
			}

			struct sockaddr_in sendAddr;
			memset(&sendAddr, 0, sizeof(sendAddr));
			sendAddr.sin_family = AF_INET;
			sendAddr.sin_port = htons(m_port);
			sendAddr.sin_addr.s_addr = inet_addr(m_mutlicastgroup.c_str());

			if (sendAddr.sin_addr.s_addr == INADDR_NONE) {
				::syslog(LOG_ERR, "Not a valid multicast IP address!");
				return ERR_NO_SUCCESS;
			}

			ssize_t nbytes = sendto(reinterpret_cast < SOCKET > (m_sendEvent.fileHandle), reinterpret_cast < const char* > (pData), length, 0, reinterpret_cast < struct sockaddr* >(&sendAddr), sizeof(sendAddr));

			if (static_cast < size_t >(nbytes) != length) {
				::syslog(LOG_ERR, "error sending message over interface %d!", interfaceIndex);
				return ERR_NO_SUCCESS;
			}

			return ERR_SUCCESS;
		}

		int MulticastServer::start(const std::string& address, unsigned int port, const DataHandler_t dataHandler)
		{
			m_mutlicastgroup = address;
			m_port = port;
			int err = setupSendSocket();
			if(err<0) {
				return err;
			}

			err = setupReceiveSocket();
			if(err<0) {
				return err;
			}

			m_dataHandler = dataHandler;
			if (dataHandler) {
				orderNextMessage();
				return m_eventLoop.addEvent(m_receiveEvent, std::bind(&MulticastServer::process, std::ref(*this)));
			}
			return 0;
		}

		void MulticastServer::stop()
		{
			dropAllInterfaces();

			if (m_sendEvent.fileHandle != INVALID_HANDLE_VALUE) {
				::closesocket(reinterpret_cast < SOCKET > (m_sendEvent.fileHandle));
				m_sendEvent.fileHandle = INVALID_HANDLE_VALUE;
			}

			if (m_receiveEvent.fileHandle != INVALID_HANDLE_VALUE) {
				::shutdown(reinterpret_cast < SOCKET > (m_receiveEvent.fileHandle), SD_BOTH);
				::closesocket(reinterpret_cast < SOCKET > (m_receiveEvent.fileHandle));
				m_receiveEvent.fileHandle = INVALID_HANDLE_VALUE;
			}

			m_eventLoop.eraseEvent(m_receiveEvent);


		}

		int MulticastServer::orderNextMessage()
		{
			DWORD recvSize;

			m_msg.name = reinterpret_cast < LPSOCKADDR >(&m_receiveAddr);
			m_msg.namelen = sizeof(m_receiveAddr);
			m_msg.lpBuffers = &m_iov;
			m_msg.lpBuffers->buf = reinterpret_cast < char* >(m_recvBuffer);
			m_msg.lpBuffers->len = static_cast <u_long >(sizeof(m_recvBuffer));

			m_msg.dwBufferCount = 1;
			m_msg.Control.buf = m_recvControlBuffer;
			m_msg.Control.len = sizeof(m_recvControlBuffer);
			m_msg.dwFlags = 0;

			if (m_wsaRecvMsg(reinterpret_cast < SOCKET > (m_receiveEvent.fileHandle), &m_msg, &recvSize, &m_receiveEvent.overlapped, NULL) != 0) {
				if (WSAGetLastError() == WSA_IO_PENDING) {
					return 0;
				} else {
					return -1;
				}
			}
			return recvSize;
		}
	}
}


