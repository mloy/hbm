// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <cstring>
#include <unistd.h>
#include <functional>

#include <syslog.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <errno.h>

#include "hbm/sys/eventloop.h"

namespace hbm {
	namespace sys {
		static const unsigned int MAXEVENTS = 16;


		EventLoop::EventLoop()
			: m_epollfd(epoll_create(1)) // parameter is ignored but must be greater than 0
			, m_stopFd(eventfd(0, EFD_NONBLOCK))
		{
			if (m_epollfd==-1) {
				throw hbm::exception::exception(std::string("epoll_create failed ") + strerror(errno));
			}

			struct epoll_event ev;
			memset(&ev, 0, sizeof(ev));
			ev.events = EPOLLIN | EPOLLET;
			ev.data.u32 = m_stopFd;
			if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_stopFd, &ev) == -1) {
				throw hbm::exception::exception(std::string("add stop notifier to eventloop failed ") + strerror(errno));
			}
		}

		EventLoop::~EventLoop()
		{
			stop();
			close(m_epollfd);
			close(m_stopFd);
		}

		int EventLoop::addEvent(event fd, const EventHandler_t &eventHandler)
		{
			if ((!eventHandler)||(fd==-1)) {
				return -1;
			}

			int mode = EPOLL_CTL_MOD;
			struct epoll_event ev;
			memset(&ev, 0, sizeof(ev));
			ev.events = EPOLLIN | EPOLLET;
			ev.data.fd = fd;
			{
				std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
				eventInfos_t::iterator eventsIter = m_eventInfos.find(fd);

				if (eventsIter==m_eventInfos.end()) {
					EventsHandlers_t eventHandlers;
					eventHandlers.inEvent = eventHandler;
					m_eventInfos.emplace(std::make_pair(fd, eventHandlers));
					mode = EPOLL_CTL_ADD;
				} else {
					EventsHandlers_t &eventHandlers = eventsIter->second;
					if (eventHandlers.outEvent) {
						ev.events |= EPOLLOUT;
					}
					eventHandlers.inEvent = eventHandler;
				}

				if (epoll_ctl(m_epollfd, mode, fd, &ev) == -1) {
					if (mode==EPOLL_CTL_MOD) {
						syslog(LOG_ERR, "epoll_ctl failed while modifying event '%s' (%d) epoll_d:%d, event_fd:%d", strerror(errno), errno, m_epollfd, fd);
					} else {
						syslog(LOG_ERR, "epoll_ctl failed while adding event '%s' (%d) epoll_d:%d, event_fd:%d", strerror(errno), errno, m_epollfd, fd);
					}
					return -1;
				}
			}

			// there might have been work to do before fd was added to epoll. This won't be signaled by edge triggered epoll. Try until there is nothing left.
			ssize_t result;
			do {
				result = eventHandler();
			} while (result>0);
			return 0;
		}
		
		int EventLoop::addOutEvent(event fd, const EventHandler_t &eventHandler)
		{
			if ((!eventHandler)||(fd==-1)) {
				return -1;
			}

			int mode = EPOLL_CTL_MOD;
			struct epoll_event ev;
			memset(&ev, 0, sizeof(ev));
			ev.events = EPOLLOUT | EPOLLET;
			ev.data.fd = fd;
			{
				std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
				eventInfos_t::iterator eventsIter = m_eventInfos.find(fd);

				if (eventsIter==m_eventInfos.end()) {
					EventsHandlers_t eventHandlers;
					eventHandlers.outEvent = eventHandler;
					m_eventInfos.emplace(std::make_pair(fd, eventHandlers));
					mode = EPOLL_CTL_ADD;
				} else {
					EventsHandlers_t &eventHandlers = eventsIter->second;
					if (eventHandlers.inEvent) {
						ev.events |= EPOLLIN;
					}
					eventHandlers.outEvent = eventHandler;
				}

				if (epoll_ctl(m_epollfd, mode, fd, &ev) == -1) {
					if (mode==EPOLL_CTL_MOD) {
						syslog(LOG_ERR, "epoll_ctl failed while modifying event '%s' (%d) epoll_d:%d, event_fd:%d", strerror(errno), errno, m_epollfd, fd);
					} else {
						syslog(LOG_ERR, "epoll_ctl failed while adding event '%s' (%d) epoll_d:%d, event_fd:%d", strerror(errno), errno, m_epollfd, fd);
					}
					return -1;
				}
			}

			// there might have been work to do before fd was added to epoll. This won't be signaled by edge triggered epoll. Try until there is nothing left.
			ssize_t result;
			do {
				result = eventHandler();
			} while (result>0);
			return 0;
		}

		int EventLoop::eraseEvent(event fd)
		{
			std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
			int ret;
			eventInfos_t::iterator eventsIter = m_eventInfos.find(fd);
			if ( eventsIter==m_eventInfos.end()) {
				return -1;
			}
			EventsHandlers_t &eventHandlers = eventsIter->second;
			if (eventHandlers.outEvent) {
				// keep the existing EPOLLOUT event
				struct epoll_event ev;
				memset(&ev, 0, sizeof(ev));
				ev.events = EPOLLOUT | EPOLLET;
				ev.data.fd = fd;
				ret = epoll_ctl(m_epollfd, EPOLL_CTL_MOD, fd, &ev);
			} else {
				ret = epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, nullptr);
				m_eventInfos.erase(eventsIter);
			}
			return ret;
		}

		int EventLoop::eraseOutEvent(event fd)
		{
			std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
			int ret;
			eventInfos_t::iterator eventsIter = m_eventInfos.find(fd);
			if ( eventsIter==m_eventInfos.end()) {
				return -1;
			}
			EventsHandlers_t &eventHandlers = eventsIter->second;
			if (eventHandlers.inEvent) {
				// keep the existing EPOLLIN event
				struct epoll_event ev;
				memset(&ev, 0, sizeof(ev));
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = fd;
				ret = epoll_ctl(m_epollfd, EPOLL_CTL_MOD, fd, &ev);
			} else {
				ret = epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, nullptr);
				m_eventInfos.erase(eventsIter);
			}
			return ret;
		}
		
		int EventLoop::execute()
		{
			int nfds;
			struct epoll_event events[MAXEVENTS];
			ssize_t result;
			int fd;

			while (true) {
				do {
					nfds = epoll_wait(m_epollfd, events, MAXEVENTS, -1);
				} while ((nfds==-1) && (errno==EINTR));
				
				if (nfds<=0) {
					return nfds;
				}

				{
					std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
					// We are working edge triggered, hence we need to process everything that is available for each event.
					// To be fair, the callback of each signaled event is called only once. 
					// After all the callbacks of all signaled events were called, we start from the beginning until no signaled event is left.
					unsigned int eventsLeft;
					do {
						eventsLeft = 0;
						for (int n = 0; n < nfds; ++n) {
							fd = events[n].data.fd;
							eventInfos_t::iterator iter = m_eventInfos.find(fd);
							if (iter!=m_eventInfos.end()) {
								// we are working edge triggered, hence we need to read everything that is available
								if (events[n].events & EPOLLIN) {
									try {
										result = iter->second.inEvent();
										if (result>0) {
											// there might be more to read...
											++eventsLeft;
										} else {
											// we are done with this event
											events[n].events &= ~EPOLLIN;
										}
									} catch (...) {
										// ignore
									}
								}
								if (events[n].events & EPOLLOUT) {
									try {
										result = iter->second.outEvent();
										if (result>0) {
											// there might be more to write...
											++eventsLeft;
										} else {
											// we are done with this event
											events[n].events &= ~EPOLLOUT;
										}
									} catch (...) {
										// ignore
									}
								}
							} else {
								if (fd==m_stopFd) {
									// stop notification!
									return 0;
								}
							}
						}
					} while (eventsLeft);
				}
			}
		}

		void EventLoop::stop()
		{
			static const uint64_t value = 1;
			if (write(m_stopFd, &value, sizeof(value))<0) {
				syslog(LOG_ERR, "notifying stop of eventloop failed");
			}
		}
	}
}
