// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#include <WinSock2.h>
#include <IPHlpApi.h>

#include <cstring>

#include "hbm/communication/netlink.h"
#include "hbm/exception/exception.hpp"


namespace hbm {
	namespace communication {
		Netlink::Netlink(communication::NetadapterList &netadapterlist, sys::EventLoop &eventLoop)
			: m_netadapterlist(netadapterlist)
			, m_eventloop(eventLoop)
		{
			m_event.overlapped.hEvent = WSACreateEvent();
			if (orderNextEvent() == -1) {
				throw hbm::exception::exception("NotifyAddrChange failed: " + WSAGetLastError());
			}
		}

		Netlink::~Netlink()
		{
			stop();
		}

		ssize_t Netlink::process()
		{
			m_netadapterlist.update();
			if (m_interfaceAddressEventHandler) {
				m_interfaceAddressEventHandler(COMPLETE, 0, "");
			}
			return orderNextEvent();
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
			return CloseHandle(m_event.overlapped.hEvent);
		}
		
		int Netlink::orderNextEvent()
		{
			HANDLE handle = NULL;
			DWORD ret = NotifyAddrChange(&handle, &m_event.overlapped);
			if (ret != NO_ERROR) {
				int error = WSAGetLastError();
				if (error != WSA_IO_PENDING) {
					return -1;
				}
			}
			return 0;
		}
	}
}
