/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2007 Hottinger Baldwin Messtechnik GmbH
 * Im Tiefen See 45
 * 64293 Darmstadt
 * Germany
 * http://www.hbm.com
 * All rights reserved
 *
 * The copyright to the computer program(s) herein is the property of
 * Hottinger Baldwin Messtechnik GmbH (HBM), Germany. The program(s)
 * may be used and/or copied only with the written permission of HBM
 * or in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 * This copyright notice must not be removed.
 *
 * This Software is licenced by the
 * "General supply and license conditions for software"
 * which is part of the standard terms and conditions of sale from HBM.
*/


#include <vector>
#include <cstring>
#include <cstdio>

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
#undef max
#undef min

#define syslog fprintf
#define sprintf sprintf_s
#define LOG_DEBUG stdout
#define LOG_INFO stdout
#define LOG_ERR stderr
#else

#include <poll.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif

#include "hbm/exception/exception.hpp"

#include "multicastserver.h"

namespace hbm {
	namespace communication {
		MulticastServer::MulticastServer(const std::string& address, unsigned int port, const NetadapterList &netadapterList)
			: m_address(address)
			, m_port(port)
			, m_ReceiveSocket(-1)
			, m_SendSocket(-1)
	#ifdef _WIN32
			, m_event(WSACreateEvent())
	#endif
			, m_receiveAddr()
			, m_netadapterList(netadapterList)
		{

	#ifdef _WIN32
			WORD RequestedSockVersion = MAKEWORD(2, 0);
			WSADATA wsaData;
			WSAStartup(RequestedSockVersion, &wsaData);
	#endif

		}

		MulticastServer::~MulticastServer()
		{
			stop();
	#ifdef _WIN32
			WSACloseEvent(m_event);
	#endif
		}

#ifdef _WIN32
		event MulticastServer::getFd() const
		{
			return m_event;
		}
#else
		event MulticastServer::getFd() const
		{
			return m_ReceiveSocket;
		}
#endif

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

				if ( getaddrinfo(m_address.c_str(), portString, &hints, &pResult) != 0 ) {
					::syslog(LOG_ERR, "Not a valid multicast IP address!");
					return -1;
				}

				m_ReceiveSocket = socket(pResult->ai_family, SOCK_DGRAM, 0);

				memset(&m_receiveAddr, 0, sizeof(m_receiveAddr));
				m_receiveAddr.sin_family = pResult->ai_family;
				m_receiveAddr.sin_addr.s_addr = htonl(INADDR_ANY);
				m_receiveAddr.sin_port = htons(m_port);

				freeaddrinfo(pResult);
			}





			if (m_ReceiveSocket < 0) {
				::syslog(LOG_ERR, "Could not create receiving socket!");
				return -1;
			}

	#ifdef _WIN32
			// WSAEventSelect automatically sets the socket to non blocking!
			WSAEventSelect(m_ReceiveSocket, m_event, FD_READ);
			// switch to blocking
			u_long value = 0;
			::ioctlsocket(m_ReceiveSocket, FIONBIO, &value);
	#endif


			// sufficient buffer for several messages
			int RcvBufSize = 128000;
	#ifdef _WIN32
			if (setsockopt(m_ReceiveSocket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast < char* >(&RcvBufSize), sizeof(RcvBufSize)) < 0) {
	#else
			if (setsockopt(m_ReceiveSocket, SOL_SOCKET, SO_RCVBUF, &RcvBufSize, sizeof(RcvBufSize)) < 0) {
	#endif
				return -1;
			}

			uint32_t yes = 1;
			// allow multiple sockets to use the same PORT number
	#ifdef _WIN32
			if (setsockopt(m_ReceiveSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast < char* >(&yes), sizeof(yes)) < 0) {
	#else

			if (setsockopt(m_ReceiveSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
	#endif
				::syslog(LOG_ERR, "Could not set SO_REUSEADDR!");
				return -1;
			}

			// We do want to know the interface, we received on
			if (setsockopt(m_ReceiveSocket, IPPROTO_IP, IP_PKTINFO, reinterpret_cast < char* >(&yes), sizeof(yes)) != 0) {
				::syslog(LOG_ERR, "Could not set IP_PKTINFO!");
				return -1;
			}

			//if (setsockopt(m_ReceiveSocket, IPPROTO_IP, IP_RECVTTL, reinterpret_cast < char* >(&yes), sizeof(yes)) != 0) {
			//	::syslog(LOG_ERR, "Could not set IP_RECVTTL!");
			//	return -1;
			//}

			if (bind(m_ReceiveSocket, (struct sockaddr*)&m_receiveAddr, sizeof(m_receiveAddr)) < 0) {
				::syslog(LOG_ERR, "Could not bind socket! Error %s", strerror(errno));
				return -1;
			}

			return 0;
		}


		int MulticastServer::setupSendSocket()
		{
			m_SendSocket = socket(AF_INET6, SOCK_DGRAM, 0);

			if (m_SendSocket < 0) {
				::syslog(LOG_ERR, "Could not create socket!");
				return -1;
			}


			// on simulation we want to be able to have scan daemon and ckient on the same machine. Hence we need to receive the stuff send by ourselves.
	#ifdef _HBM_HARDWARE
			{
				// we do not want to receive the stuff we where sending
				unsigned char value = 0;
	#ifdef _WIN32
				if (setsockopt(m_SendSocket, IPPROTO_IP, IP_MULTICAST_LOOP, reinterpret_cast <char* > (&value), sizeof(value))) {
	#else
				if (setsockopt(m_SendSocket, IPPROTO_IP, IP_MULTICAST_LOOP, &value, sizeof(value))) {
	#endif
					::syslog(LOG_ERR, "Error setsockopt IP_MULTICAST_LOOP!");
					return -1;
				}
			}
	#endif

			return 0;
		}

		ssize_t MulticastServer::receiveTelegram(void* msgbuf, size_t len, int& adapterIndex, boost::posix_time::milliseconds timeout)
		{
			int retval;
	#ifdef _WIN32
			fd_set rfds;
			FD_ZERO(&rfds);
			FD_SET(m_ReceiveSocket, &rfds);
			struct timeval waitTime;
			waitTime.tv_sec = 0;
			waitTime.tv_usec = static_cast < long > (timeout.total_milliseconds()*1000);

			retval = select(static_cast < int > (m_ReceiveSocket) + 1, &rfds, NULL, NULL, &waitTime);
	#else
			struct pollfd pfd;
			pfd.fd = m_ReceiveSocket;
			pfd.events = POLLIN;

			do {
				retval = poll(&pfd, 1, timeout.total_milliseconds());
			} while((retval==-1) && (errno==EINTR) );

			if(pfd.revents & POLLNVAL) {
				// recognize that socket is closed!
				return -1;
			}
	#endif

			if (retval > 0) {
				int ttl;
				retval = receiveTelegram(msgbuf, len, adapterIndex, ttl);
			}

			return retval;
		}

		int MulticastServer::dropInterface(const std::string& interfaceAddress)
		{
			return dropOrAddInterface(interfaceAddress, false);
		}

		int MulticastServer::addInterface(const std::string& interfaceAddress)
		{
			return dropOrAddInterface(interfaceAddress, true);
		}

		void MulticastServer::addAllInterfaces()
		{
			NetadapterList::tAdapters adapters = m_netadapterList.get();
			for (NetadapterList::tAdapters::const_iterator iter = adapters.begin(); iter != adapters.end(); ++iter) {
				const communication::Netadapter& adapter = iter->second;

				const communication::addressesWithNetmask_t& addresses = adapter.getIpv4Addresses();
				if(addresses.empty()==false) {
					addInterface(addresses.front().address);
				}
			}
		}

		void MulticastServer::dropAllInterfaces()
		{
			NetadapterList::tAdapters adapters = m_netadapterList.get();
			for (NetadapterList::tAdapters::const_iterator iter = adapters.begin(); iter != adapters.end(); ++iter) {
				const communication::Netadapter& adapter = iter->second;

				const communication::addressesWithNetmask_t& addresses = adapter.getIpv4Addresses();
				if(addresses.empty()==false) {
					dropInterface(addresses.front().address);
				}
			}
		}


		int MulticastServer::dropOrAddInterface(const std::string& interfaceAddress, bool add)
		{
			int retVal = 0;
			struct addrinfo hints;
			struct addrinfo* pResult = NULL;
			char portString[8];

	#ifdef _WIN32
			struct ip_mreq im;
	#else
			struct ip_mreqn im;
	#endif

			memset(&hints, 0, sizeof(hints));

			sprintf(portString, "%u", m_port);
			hints.ai_family   = PF_UNSPEC;
			hints.ai_socktype = SOCK_DGRAM;

			if ( getaddrinfo(m_address.c_str(), portString, &hints, &pResult) != 0 ) {
				::syslog(LOG_ERR, "Not a valid multicast IP address (%s)!", m_address.c_str());
				return -1;
			}

			memset(&im, 0, sizeof(im));
			memcpy(&im.imr_multiaddr, &(reinterpret_cast < struct sockaddr_in* > (pResult->ai_addr))->sin_addr, sizeof(im.imr_multiaddr.s_addr));

			freeaddrinfo(pResult);

	#ifdef _WIN32
			im.imr_interface.s_addr = inet_addr(interfaceAddress.c_str());
			if (im.imr_interface.s_addr == INADDR_NONE) {
				::syslog(LOG_ERR, "not a valid IP address for IP_ADD_MEMBERSHIP!");
				return -1;
			}
	#else
			if (inet_aton(interfaceAddress.c_str(), &im.imr_address) == 0) {
				::syslog(LOG_ERR, "not a valid IP address for IP_ADD_MEMBERSHIP!");
				return -1;
			}
	#endif

			int optionName;
			if(add) {
				optionName = IP_ADD_MEMBERSHIP;
			} else {
				optionName = IP_DROP_MEMBERSHIP;
			}


	#ifdef _WIN32
			retVal = setsockopt(m_ReceiveSocket, IPPROTO_IP, optionName, reinterpret_cast < char* >(&im), sizeof(im));
	#else
			retVal = setsockopt(m_ReceiveSocket, IPPROTO_IP, optionName, &im, sizeof(im));
	#endif

			if(add) {
				if(retVal!=0) {
					if(errno==EADDRINUSE) {
						// ignore already added
						return 0;
					}
					//::syslog(LOG_ERR, "Error adding interface address %s to multicast group. Error: %d '%s'!", interfaceAddress.c_str(), errno, strerror((errno)));
				}
			} else {
				if(retVal!=0) {
					if(errno==EADDRNOTAVAIL) {
						// ignore already dropped
						return 0;
					}
					//::syslog(LOG_ERR, "Error removing interface address %s from multicast group. Error: %d '%s'!", interfaceAddress.c_str(), errno, strerror((errno)));
				}
			}

			return retVal;
		}

		ssize_t MulticastServer::receiveTelegram(void* msgbuf, size_t len, int& receivingAdapterIndex, int& ttl)
		{
			// we do use recvmsg here because we get some additional information: The interface we received from.
			ttl = 1;
			char controlbuffer[100];
			ssize_t nbytes;

	#ifdef _WIN32
			LPFN_WSARECVMSG WSARecvMsg;
			DWORD winLen;

			GUID InBuffer[] = WSAID_WSARECVMSG;
			int status = WSAIoctl(m_ReceiveSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &InBuffer, sizeof(InBuffer), &WSARecvMsg, sizeof(WSARecvMsg), &winLen, NULL, NULL);

			if (status != 0) {
				return -1;
			}

			WSAMSG msg;
			WSABUF iov;
			DWORD bytes_received;

			msg.name = reinterpret_cast < LPSOCKADDR >(&m_receiveAddr);
			msg.namelen = sizeof(m_receiveAddr);
			msg.lpBuffers = &iov;
			msg.lpBuffers->buf = reinterpret_cast < char* >(msgbuf);
			msg.lpBuffers->len = static_cast <u_long >(len);
			msg.dwBufferCount = 1;
			msg.Control.buf = controlbuffer;
			msg.Control.len = sizeof(controlbuffer);
			msg.dwFlags = 0;

			if (WSARecvMsg(m_ReceiveSocket, &msg, &bytes_received, NULL, NULL) != 0) {
				if(WSAGetLastError()==WSAEWOULDBLOCK) {
					nbytes = 0;
				} else {
					nbytes = -1;
				}
			} else {
				nbytes = bytes_received;
			}

	#else
			struct msghdr msg;
			struct iovec iov;

			msg.msg_name = &m_receiveAddr;
			msg.msg_namelen = sizeof(m_receiveAddr);
			msg.msg_iov = &iov;
			msg.msg_iov->iov_base = msgbuf;
			msg.msg_iov->iov_len = len;
			msg.msg_iovlen = 1;
			msg.msg_control = &controlbuffer;
			msg.msg_controllen = sizeof(controlbuffer);
			msg.msg_flags = 0;
			nbytes = ::recvmsg(m_ReceiveSocket, &msg, 0);
	#endif

			if (nbytes > 0) {
	#ifdef _WIN32
				for (WSACMSGHDR* pcmsghdr = WSA_CMSG_FIRSTHDR(&msg); pcmsghdr != NULL; pcmsghdr = WSA_CMSG_NXTHDR(&msg, pcmsghdr))
	#else
				for (struct cmsghdr* pcmsghdr = CMSG_FIRSTHDR(&msg); pcmsghdr != NULL; pcmsghdr = CMSG_NXTHDR(&msg, pcmsghdr))
	#endif
				{
					if (pcmsghdr->cmsg_type == IP_PKTINFO) {
						struct in_pktinfo* ppktinfo;
	#ifdef _WIN32
						ppktinfo = reinterpret_cast <struct in_pktinfo*> (WSA_CMSG_DATA(pcmsghdr));
	#else
						ppktinfo = reinterpret_cast <struct in_pktinfo*> (CMSG_DATA(pcmsghdr));
	#endif
						receivingAdapterIndex = ppktinfo->ipi_ifindex;
					} else if(pcmsghdr->cmsg_type == IP_TTL) {
						int* pTtl;
						// returns the ttl from the received ip header (the value set by the last sender(router))
	#ifdef _WIN32
						pTtl = reinterpret_cast <int*> (WSA_CMSG_DATA(pcmsghdr));
	#else
						pTtl = reinterpret_cast <int*> (CMSG_DATA(pcmsghdr));
	#endif
						ttl = *pTtl;
					}
				}
			}
			return nbytes;
		}

		void MulticastServer::send(const void* pData, size_t length, unsigned int ttl) const
		{
			NetadapterList::tAdapters adapters = m_netadapterList.get();

			for (NetadapterList::tAdapters::const_iterator iter = adapters.begin(); iter != adapters.end(); ++iter) {
				const Netadapter& adapter = iter->second;

				sendOverInterfaceByInterfaceIndex(adapter.getIndex(), pData, length, ttl);
			}
		}

		void MulticastServer::send(const std::string& data, unsigned int ttl) const
		{
			send(data.c_str(), data.length(), ttl);
		}

		int MulticastServer::sendOverInterfaceByInterfaceIndex(int interfaceIndex, const std::string& data, unsigned int ttl) const
		{
			return sendOverInterfaceByInterfaceIndex(interfaceIndex, data.c_str(), data.length());
		}

		int MulticastServer::sendOverInterfaceByInterfaceIndex(int interfaceIndex, const void* pData, size_t length, unsigned int ttl) const
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


	#ifdef _WIN32
			if (setsockopt(m_SendSocket, IPPROTO_IPV6, IPV6_MULTICAST_IF, reinterpret_cast < char* >(&interfaceIndex), sizeof(interfaceIndex))) {
				return ERR_INVALIDADAPTER;
			}
	#else
			if (setsockopt(m_SendSocket, IPPROTO_IPV6, IPV6_MULTICAST_IF, &interfaceIndex, sizeof(interfaceIndex))) {
				::syslog(LOG_ERR, "Error '%s' setsockopt IPV6_MULTICAST_IF for interface %d!", strerror(errno), interfaceIndex);
				return ERR_INVALIDADAPTER;
			}
	#endif

	#ifdef _WIN32
			if (setsockopt(m_SendSocket, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast < char* >(&ttl), sizeof(ttl))) {
	#else

			if (setsockopt(m_SendSocket, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl))) {
	#endif
				::syslog(LOG_ERR, "Error setsockopt IP_MULTICAST_TTL to %ufor interface %d!", ttl, interfaceIndex);
				return ERR_NO_SUCCESS;
			}

			struct sockaddr_in sendAddr;
			memset(&sendAddr, 0, sizeof(sendAddr));
			sendAddr.sin_family = AF_INET;
			sendAddr.sin_port = htons(m_port);
	#ifdef _WIN32
			sendAddr.sin_addr.s_addr = inet_addr(m_address.c_str());

			if (sendAddr.sin_addr.s_addr == INADDR_NONE) {
	#else

			if (inet_aton(m_address.c_str(), &sendAddr.sin_addr) == 0) {
	#endif
				::syslog(LOG_ERR, "Not a valid multicast IP address!");
				return ERR_NO_SUCCESS;
			}

	#ifdef _WIN32
			ssize_t nbytes = sendto(m_SendSocket, reinterpret_cast < const char* > (pData), static_cast < int > (length), 0, reinterpret_cast < struct sockaddr* >(&sendAddr), sizeof(sendAddr));
	#else
			ssize_t nbytes = sendto(m_SendSocket, pData, length, 0, reinterpret_cast < struct sockaddr* >(&sendAddr), sizeof(sendAddr));
	#endif

			if (static_cast < size_t >(nbytes) != length) {
				::syslog(LOG_ERR, "error sending message over interface %d!", interfaceIndex);
				return ERR_NO_SUCCESS;
			}

			return ERR_SUCCESS;
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


	#ifdef _WIN32
			ifAddr.s_addr = inet_addr(interfaceIp.c_str());

			if (ifAddr.s_addr == INADDR_NONE) {
	#else

			if (inet_aton(interfaceIp.c_str(), &ifAddr) == 0) {
	#endif
				::syslog(LOG_ERR, "%s is not a valid interface IP address!", interfaceIp.c_str());
				return ERR_INVALIDIPADDRESS;
			}

	#ifdef _WIN32
			if (setsockopt(m_SendSocket, IPPROTO_IP, IP_MULTICAST_IF, reinterpret_cast < char* >(&ifAddr), sizeof(ifAddr))) {
	#else

			if (setsockopt(m_SendSocket, IPPROTO_IP, IP_MULTICAST_IF, &ifAddr, sizeof(ifAddr))) {
	#endif
				::syslog(LOG_ERR, "Error setsockopt IP_MULTICAST_IF for interface %s!", interfaceIp.c_str());
				return ERR_INVALIDADAPTER;
			};

	#ifdef _WIN32
			if (setsockopt(m_SendSocket, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast < char* >(&ttl), sizeof(ttl))) {
	#else

			if (setsockopt(m_SendSocket, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl))) {
	#endif
				::syslog(LOG_ERR, "%s: Error setsockopt IPV6_MULTICAST_HOPS to %u!", interfaceIp.c_str(), ttl);
				return ERR_NO_SUCCESS;
			}

			struct sockaddr_in sendAddr;
			memset(&sendAddr, 0, sizeof(sendAddr));
			sendAddr.sin_family = AF_INET;
			sendAddr.sin_port = htons(m_port);
	#ifdef _WIN32
			sendAddr.sin_addr.s_addr = inet_addr(m_address.c_str());

			if (sendAddr.sin_addr.s_addr == INADDR_NONE) {
	#else

			if (inet_aton(m_address.c_str(), &sendAddr.sin_addr) == 0) {
	#endif
				::syslog(LOG_ERR, "Not a valid multicast IP address!");
				return ERR_NO_SUCCESS;
			}

	#ifdef _WIN32
			ssize_t nbytes = sendto(m_SendSocket, reinterpret_cast < const char* > (pData), static_cast < int > (length), 0, reinterpret_cast < struct sockaddr* >(&sendAddr), sizeof(sendAddr));
	#else
			ssize_t nbytes = sendto(m_SendSocket, pData, length, 0, reinterpret_cast < struct sockaddr* >(&sendAddr), sizeof(sendAddr));
	#endif

			if (static_cast < size_t >(nbytes) != length) {
				::syslog(LOG_ERR, "error sending message over interface %s!", interfaceIp.c_str());
				return ERR_NO_SUCCESS;
			}

			return ERR_SUCCESS;
		}


		int MulticastServer::start()
		{
			int err = setupSendSocket();
			if(err<0) {
				return err;
			}

			err = setupReceiveSocket();
			return err;
		}

		void MulticastServer::stop()
		{
	#ifdef _WIN32
			::closesocket(m_SendSocket);
	#else
			::close(m_SendSocket);
	#endif

	#ifdef _WIN32
			::shutdown(m_ReceiveSocket, SD_BOTH);
			::closesocket(m_ReceiveSocket);
	#else
			::shutdown(m_ReceiveSocket, SHUT_RDWR);
			::close(m_ReceiveSocket);
	#endif
		}
	}
}
