// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


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
#include "hbm/sys/eventloop.h"

#include "hbm/communication/multicastserver.h"

namespace hbm {
	namespace communication {
		MulticastServer::MulticastServer(NetadapterList& netadapterList, sys::EventLoop &eventLoop)
			: m_address()
			, m_port()
			, m_ReceiveSocket(NO_SOCKET)
			, m_SendSocket(NO_SOCKET)
			, m_receiveAddr()
			, m_netadapterList(netadapterList)
			, m_eventLoop(eventLoop)
			, m_dataHandler()
		{

		}

		MulticastServer::~MulticastServer()
		{
			stop();
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

				if ( getaddrinfo(m_address.c_str(), portString, &hints, &pResult) != 0 ) {
					::syslog(LOG_ERR, "Not a valid multicast IP address!");
					return -1;
				}

				m_ReceiveSocket = socket(pResult->ai_family, SOCK_DGRAM | SOCK_NONBLOCK, 0);
				if (m_ReceiveSocket < 0) {
					m_ReceiveSocket = NO_SOCKET;
				}

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



			// sufficient buffer for several messages
			int RcvBufSize = 128000;
			if (setsockopt(m_ReceiveSocket, SOL_SOCKET, SO_RCVBUF, &RcvBufSize, sizeof(RcvBufSize)) < 0) {
				return -1;
			}

			uint32_t yes = 1;
			// allow multiple sockets to use the same PORT number

			if (setsockopt(m_ReceiveSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
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
				::syslog(LOG_ERR, "Could not bind socket!");
				return -1;
			}

			return 0;
		}


		int MulticastServer::setupSendSocket()
		{
			m_SendSocket = socket(AF_INET, SOCK_DGRAM, 0);

			if (m_SendSocket < 0) {
				m_SendSocket = NO_SOCKET;
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

			if (setsockopt(m_SendSocket, IPPROTO_IP, IP_MULTICAST_LOOP, &param, sizeof(param))) {
				::syslog(LOG_ERR, "Error setsockopt IP_MULTICAST_LOOP '%s'!", strerror(errno));
				return -1;
			}
			return 0;
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

			struct ip_mreqn im;

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

			if (inet_aton(interfaceAddress.c_str(), &im.imr_address) == 0) {
				::syslog(LOG_ERR, "'%s' not a valid IP address for IP_ADD_MEMBERSHIP!", interfaceAddress.c_str());
				return -1;
			}

			int optionName;
			if(add) {
				optionName = IP_ADD_MEMBERSHIP;
			} else {
				optionName = IP_DROP_MEMBERSHIP;
			}


			retVal = setsockopt(m_ReceiveSocket, IPPROTO_IP, optionName, &im, sizeof(im));

			if(add) {
				if(retVal!=0) {
					if(errno==EADDRINUSE) {
						// ignore already added
						return 0;
					}

				}
			} else {
				if(retVal!=0) {
					if (errno == EADDRNOTAVAIL) {
						// ignore already dropped
						return 0;
					}
				}
			}

			return retVal;
		}

		int MulticastServer::process()
		{
			if (m_dataHandler) {
				return m_dataHandler(this);
			} else {
				return -1;
			}
		}

		ssize_t MulticastServer::receiveTelegram(void* msgbuf, size_t len, Netadapter& adapter, int& ttl)
		{
			int interfaceIndex = 0;
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
			int interfaceIndex = 0;
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

		ssize_t MulticastServer::receiveTelegram(void* msgbuf, size_t len, int& adapterIndex, int& ttl)
		{
			// we do use recvmsg here because we get some additional information: The interface we received from.
			ttl = 1;
			char controlbuffer[100];
			ssize_t nbytes;

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

			if (nbytes > 0) {
				for (struct cmsghdr* pcmsghdr = CMSG_FIRSTHDR(&msg); pcmsghdr != NULL; pcmsghdr = CMSG_NXTHDR(&msg, pcmsghdr))
				{
					if (pcmsghdr->cmsg_type == IP_PKTINFO) {
						struct in_pktinfo* ppktinfo;
						ppktinfo = reinterpret_cast <struct in_pktinfo*> (CMSG_DATA(pcmsghdr));
						adapterIndex = ppktinfo->ipi_ifindex;
					} else if(pcmsghdr->cmsg_type == IP_TTL) {
						int* pTtl;
						// returns the ttl from the received ip header (the value set by the last sender(router))
						pTtl = reinterpret_cast <int*> (CMSG_DATA(pcmsghdr));
						ttl = *pTtl;
					}
				}
			}
			return nbytes;
		}

		int MulticastServer::send(const void* pData, size_t length, unsigned int ttl) const
		{
			int retVal = 0;
			int retValIntern;

			NetadapterList::tAdapters adapters = m_netadapterList.get();

			for (NetadapterList::tAdapters::const_iterator iter = adapters.begin(); iter != adapters.end(); ++iter) {
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

			const communication::addressesWithNetmask_t addressesWithNetmask = adapter.getIpv4Addresses();
			if(addressesWithNetmask.empty()) {
				return communication::ERR_ADAPTERISDOWN;
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
					const communication::addressesWithNetmask_t addressesWithNetmask = adapter.getIpv4Addresses();
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



			if (inet_aton(interfaceIp.c_str(), &ifAddr) == 0) {
				::syslog(LOG_ERR, "%s is not a valid interface IP address!", interfaceIp.c_str());
				return ERR_INVALIDIPADDRESS;
			}


			if (setsockopt(m_SendSocket, IPPROTO_IP, IP_MULTICAST_IF, &ifAddr, sizeof(ifAddr))) {
				::syslog(LOG_ERR, "Error setsockopt IP_MULTICAST_IF for interface %s!", interfaceIp.c_str());
				return ERR_INVALIDADAPTER;
			};


			if (setsockopt(m_SendSocket, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl))) {
				::syslog(LOG_ERR, "%s: Error setsockopt IPV6_MULTICAST_HOPS to %u!", interfaceIp.c_str(), ttl);
				return ERR_NO_SUCCESS;
			}

			struct sockaddr_in sendAddr;
			memset(&sendAddr, 0, sizeof(sendAddr));
			sendAddr.sin_family = AF_INET;
			sendAddr.sin_port = htons(m_port);

			if (inet_aton(m_address.c_str(), &sendAddr.sin_addr) == 0) {
				::syslog(LOG_ERR, "Not a valid multicast IP address!");
				return ERR_NO_SUCCESS;
			}

			ssize_t nbytes = sendto(m_SendSocket, pData, length, 0, reinterpret_cast < struct sockaddr* >(&sendAddr), sizeof(sendAddr));

			if (static_cast < size_t >(nbytes) != length) {
				::syslog(LOG_ERR, "error sending message over interface %s!", interfaceIp.c_str());
				return ERR_NO_SUCCESS;
			}

			return ERR_SUCCESS;
		}


		int MulticastServer::start(const std::string& address, unsigned int port, const DataHandler_t dataHandler)
		{
			m_address = address;
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
				m_eventLoop.addEvent(m_ReceiveSocket, std::bind(&MulticastServer::process, this));
			}
			return 0;
		}

		void MulticastServer::stop()
		{
			dropAllInterfaces();
			m_eventLoop.eraseEvent(m_ReceiveSocket);


			if (m_SendSocket != NO_SOCKET) {
				::close(m_SendSocket);
				m_SendSocket = NO_SOCKET;
			}

			if (m_ReceiveSocket != NO_SOCKET) {

				::shutdown(m_ReceiveSocket, SHUT_RDWR);
				::close(m_ReceiveSocket);
				m_ReceiveSocket = NO_SOCKET;
			}
		}
	}
}
