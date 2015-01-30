// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <iostream>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <functional>

#include <syslog.h>
#include <sys/epoll.h>

#include <errno.h>


#include "hbm/sys/eventloop.h"
#include "hbm/sys/notifier.h"

namespace hbm {
	namespace sys {
		EventLoop::EventLoop()
			: m_epollfd(epoll_create(1)) // parameter is ignored but must be greater than 0
		{
			if(m_epollfd==-1) {
				throw hbm::exception::exception(std::string("epoll_create failed)") + strerror(errno));
			}

			m_stopEvent.fd = m_stopNotifier.getFd();

			struct epoll_event ev;
			ev.events = EPOLLIN;
			ev.data.ptr = nullptr;
			if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_stopEvent.fd, &ev) == -1) {
				syslog(LOG_ERR, "epoll_ctl failed %s", strerror(errno));
			}
		}

		EventLoop::~EventLoop()
		{
			close(m_epollfd);
		}

		void EventLoop::addEvent(event fd, eventHandler_t eventHandler)
		{
			eraseEvent(fd);

			eventInfo_t evi;
			evi.fd = fd;
			evi.eventHandler = eventHandler;

			{
				std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
				eventInfo_t& eviRef = m_eventInfos[fd] = evi;

				struct epoll_event ev;
				ev.events = EPOLLIN;
				ev.data.ptr = &eviRef;
				if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
					syslog(LOG_ERR, "epoll_ctl failed %s", strerror(errno));
				}
			}
		}

		void EventLoop::eraseEvent(event fd)
		{
			std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
			eventInfos_t::iterator iter = m_eventInfos.find(fd);
			if(iter!=m_eventInfos.end()) {
				const eventInfo_t& eviRef = iter->second;
				epoll_ctl(m_epollfd, EPOLL_CTL_DEL, eviRef.fd, NULL);
				m_eventInfos.erase(iter);
			}
		}

		void EventLoop::clear()
		{
			std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
			for (eventInfos_t::iterator iter = m_eventInfos.begin(); iter!=m_eventInfos.end(); ++iter) {
				const eventInfo_t& eviRef = iter->second;
				epoll_ctl(m_epollfd, EPOLL_CTL_DEL, eviRef.fd, NULL);
			}
			m_eventInfos.clear();
		}

		int EventLoop::execute()
		{
			int nfds;
			static const unsigned int MAXEVENTS = 16;
			struct epoll_event events[MAXEVENTS];

			while (true) {
				do {
					nfds = epoll_wait(m_epollfd, events, MAXEVENTS, -1);
				} while ((nfds==-1) && (errno==EINTR));

				if((nfds==0) || (nfds==-1)) {
					// 0: time out!
					return nfds;
				}

				for (int n = 0; n < nfds; ++n) {
					if(events[n].events & EPOLLIN) {
						std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
						eventInfo_t* pEventInfo = reinterpret_cast < eventInfo_t* > (events[n].data.ptr);
						if(pEventInfo==nullptr) {
							// stop notification!
							return 0;
						}
						ssize_t result = pEventInfo->eventHandler();
						if(result<0) {
							return result;
						}
					}
				}
			}
		}

		int EventLoop::execute_for(std::chrono::milliseconds timeToWait)
		{
			int timeout;
			std::chrono::steady_clock::time_point endTime;
			if(timeToWait!=std::chrono::milliseconds(0)) {
				endTime = std::chrono::steady_clock::now() + timeToWait;
			}

			int nfds;
			static const unsigned int MAXEVENTS = 16;
			struct epoll_event events[MAXEVENTS];

			while (true) {
				std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
				if(now>=endTime) {
					return 0;
				}
				std::chrono::milliseconds timediff = std::chrono::duration_cast < std::chrono::milliseconds > (endTime-now);

				timeout = static_cast< int > (timediff.count());

				do {
					nfds = epoll_wait(m_epollfd, events, MAXEVENTS, timeout);
				} while ((nfds==-1) && (errno==EINTR));

				if((nfds==0) || (nfds==-1)) {
					// 0: time out!
					return nfds;
				}

				for (int n = 0; n < nfds; ++n) {
					if(events[n].events & EPOLLIN) {
						eventInfo_t* pEventInfo = reinterpret_cast < eventInfo_t* > (events[n].data.ptr);
						if(pEventInfo==nullptr) {
							// stop notification!
							return 0;
						}
						ssize_t result = pEventInfo->eventHandler();
						if(result<0) {
							return result;
						}
					}
				}
			}
		}

		void EventLoop::stop()
		{
			m_stopNotifier.notify();
		}
	}
}
