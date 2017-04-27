// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <iostream>


#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>



#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <net/if.h>

#include <syslog.h>
#include <stdint.h>

#include <unistd.h>

#include <cstring>

#include "hbm/communication/netlink.h"
#include "hbm/exception/exception.hpp"

const unsigned int MAX_DATAGRAM_SIZE = 65536;


namespace hbm {
	namespace communication {
		Netlink::Netlink(communication::NetadapterList &netadapterlist, sys::EventLoop &eventLoop)
			: m_event(socket(AF_NETLINK, SOCK_RAW | SOCK_NONBLOCK, NETLINK_ROUTE))
			, m_netadapterlist(netadapterlist)
			, m_eventloop(eventLoop)
		{
			if (m_event<0) {
				throw hbm::exception::exception("could not open netlink socket");
			}


			int yes = 1;
			// allow multiple sockets to use the same PORT number
			if (setsockopt(m_event, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
				throw hbm::exception::exception("Could not set SO_REUSEADDR!");
			}




			struct sockaddr_nl netLinkAddr;

			memset(&netLinkAddr, 0, sizeof(netLinkAddr));

			netLinkAddr.nl_family = AF_NETLINK;
			// setting to zero causes the kernel to choose. important for having several netlink fds in one process
			netLinkAddr.nl_pid = 0;
			netLinkAddr.nl_groups = RTMGRP_LINK | RTMGRP_NOTIFY | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR | RTMGRP_IPV4_ROUTE;

			if (::bind(m_event, reinterpret_cast < struct sockaddr *> (&netLinkAddr), sizeof(netLinkAddr))<0) {
				throw hbm::exception::exception(std::string("could not bind netlink socket '")+strerror(errno)+"'");
			}
		}

		Netlink::~Netlink()
		{
			stop();
		}

		ssize_t Netlink::receive(void *pReadBuffer, size_t bufferSize) const
		{
			struct sockaddr_nl nladdr;
			struct iovec iov = {
				pReadBuffer,
						bufferSize
			};
			struct msghdr msg;
			memset(&msg, 0, sizeof(msg));
			msg.msg_name = &nladdr;
			msg.msg_namelen = sizeof(nladdr);
			msg.msg_iov = &iov;
			msg.msg_iovlen = 1;
			return ::recvmsg(m_event, &msg, 0);
		}

		void Netlink::processNetlinkTelegram(void *pReadBuffer, size_t bufferSize) const
		{
			
			
			/// You can get link up/down events via netlink. You would create a PF_NETLINK socket, bind to the RTMGRP_LINK group, 
			/// and wait for RTM_NEWLINK/RTM_DELLINK messages. Link up events have IFF_RUNNING and IFF_LOWER_UP set in ifinfomsg.
			/// flags, while link down events have both flags cleared (funnily enough, I'm getting RTM_NEWLINK messages for both type of events).
			
			
			
			
			for (struct nlmsghdr *nh = reinterpret_cast <struct nlmsghdr *> (pReadBuffer); NLMSG_OK (nh, bufferSize); nh = NLMSG_NEXT (nh, bufferSize)) {
				if (nh->nlmsg_type == NLMSG_DONE) {
					// The end of multipart message.
					break;
				} else if (nh->nlmsg_type == NLMSG_ERROR) {
					::syslog(LOG_ERR, "error processing netlink events");
					break;
				} else {
					m_netadapterlist.update();
					switch(nh->nlmsg_type) {
// this works but we do not need it in the moment!
//						case RTM_NEWLINK:
//						case RTM_DELLINK:
//							// we get RTM_NEWLINK if cable gets plugged or unplugged!
//							{
//								struct ifinfomsg *pifinfomsg = reinterpret_cast <struct ifinfomsg*> (NLMSG_DATA(nh));
//								if ((IFF_RUNNING & pifinfomsg->ifi_flags)==0) {
//									if (m_interfaceAddressEventHandler) {
//										m_interfaceAddressEventHandler(INTERFACE_DISCONNECTED, pifinfomsg->ifi_index, "");
//									}
//								}
//							}
						
//							break;
						case RTM_NEWADDR:
							{
								struct ifaddrmsg* pIfaddrmsg = reinterpret_cast <struct ifaddrmsg*> (NLMSG_DATA(nh));
								if(pIfaddrmsg->ifa_family==AF_INET) {
									struct rtattr *rth = IFA_RTA(pIfaddrmsg);
									int rtl = IFA_PAYLOAD(nh);
									while (rtl && RTA_OK(rth, rtl)) {
										if (rth->rta_type == IFA_LOCAL) {
											// this is to be ignored if there are more than one ipv4 addresses assigned to the interface!
											if (m_interfaceAddressEventHandler) {
												struct in_addr* pIn = reinterpret_cast < struct in_addr* > (RTA_DATA(rth));
												m_interfaceAddressEventHandler(ADDRESS_ADDED, pIfaddrmsg->ifa_index, inet_ntoa(*pIn));
											}
										}
										rth = RTA_NEXT(rth, rtl);
									}
								}
							}
							break;
						case RTM_DELADDR:
							{
								struct ifaddrmsg* pIfaddrmsg = reinterpret_cast <struct ifaddrmsg*> (NLMSG_DATA(nh));
								if(pIfaddrmsg->ifa_family==AF_INET) {
									struct rtattr *rth = IFA_RTA(pIfaddrmsg);
									int rtl = IFA_PAYLOAD(nh);
									while (rtl && RTA_OK(rth, rtl)) {
										if (rth->rta_type == IFA_LOCAL) {
											if (m_interfaceAddressEventHandler) {
												struct in_addr* pIn = reinterpret_cast < struct in_addr* > (RTA_DATA(rth));
												m_interfaceAddressEventHandler(ADDRESS_REMOVED, pIfaddrmsg->ifa_index, inet_ntoa(*pIn));
											}
										}
										rth = RTA_NEXT(rth, rtl);
									}
								}
							}
							break;
						default:
							break;
					}
				}
			}
		}

		ssize_t Netlink::process()
		{
			uint8_t readBuffer[MAX_DATAGRAM_SIZE];
			ssize_t nBytes = receive(readBuffer, sizeof(readBuffer));
			if (nBytes>0) {
				processNetlinkTelegram(readBuffer, nBytes);
			}
			return nBytes;
		}


		int Netlink::start(interfaceAddressCb_t interfaceAddressEventHandler)
		{
			m_interfaceAddressEventHandler = interfaceAddressEventHandler;
			if (m_interfaceAddressEventHandler) {
				m_interfaceAddressEventHandler(COMPLETE, 0, "");
			}

			m_eventloop.addEvent(m_event, std::bind(&Netlink::process, this));
			return 0;
		}

		int Netlink::stop()
		{
			m_eventloop.eraseEvent(m_event);
			return ::close(m_event);
		}
	}
}

