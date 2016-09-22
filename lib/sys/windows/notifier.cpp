// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#include <iostream>
#include <cstring>

#include <WinSock2.h>

#include "hbm/sys/notifier.h"
#include "hbm/sys/eventloop.h"

namespace hbm {
	namespace sys {
		Notifier::Notifier(EventLoop& eventLoop)
			: m_fd()
			, m_eventLoop(eventLoop)
			, m_eventHandler()
		{
			m_fd.completionPort = m_eventLoop.getCompletionPort();
			m_fd.overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			//m_eventLoop.addEvent(m_fd, std::bind(&Notifier::process, std::ref(*this)));
		}

		Notifier::~Notifier()
		{
			m_eventLoop.eraseEvent(m_fd);
			CloseHandle(m_fd.overlapped.hEvent);
		}


		int Notifier::notify()
		{
			if (PostQueuedCompletionStatus(m_fd.completionPort, 0, 0, &m_fd.overlapped)) {
				return 0;
			} else {
				return -1;
			}
		}

		int Notifier::process()
		{
			if (m_eventHandler) {
				m_eventHandler();
			}
			return 0;
		}

		int Notifier::set(Cb_t eventHandler)
		{
			m_eventHandler = eventHandler;
			return 0;
		}
	}
}
