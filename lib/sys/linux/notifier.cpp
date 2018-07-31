// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#include <cstring>

#include <sys/eventfd.h>
#include <unistd.h>

#include "hbm/exception/exception.hpp"
#include "hbm/sys/notifier.h"
#include "hbm/sys/eventloop.h"

namespace hbm {
	namespace sys {
		Notifier::Notifier(EventLoop& eventLoop)
			: m_fd(eventfd(0, EFD_NONBLOCK))
			, m_eventLoop(eventLoop)
			, m_eventHandler()
		{
			if (m_fd<0) {
				throw hbm::exception::exception("could not create event fd");
			}
			
			if (m_eventLoop.addEvent(m_fd, std::bind(&Notifier::process, this))<0) {
				throw hbm::exception::exception("could not add timer to event loop");
			}
		}

		Notifier::~Notifier()
		{
			m_eventLoop.eraseEvent(m_fd);
			close(m_fd);
		}

		int Notifier::set(Cb_t eventHandler)
		{
			m_eventHandler = eventHandler;
			return 0;
		}


		int Notifier::notify()
		{
			static const uint64_t value = 1;
			return write(m_fd, &value, sizeof(value));
		}

		int Notifier::process()
		{
			uint64_t eventCount = 0;
			// it is sufficient to read once in order to rearm
			::read(m_fd, &eventCount, sizeof(eventCount));
//			uint64_t eventCountSum = 0;
//			uint64_t eventCount = 0;
//			while (::read(m_fd, &eventCount, sizeof(eventCount))==sizeof(eventCount)) {
//				if (eventCount>0) {
//					eventCountSum += eventCount;
//				}
//			}
//			for (uint64_t i=0; i<eventCountSum; i++) {
			for (uint64_t i=0; i<eventCount; i++) {
				if (m_eventHandler) {
					m_eventHandler();
				}
			}
			return 0;
		}
	}
}
