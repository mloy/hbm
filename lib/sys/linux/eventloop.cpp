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


static const uint64_t notifyValue = 1;

namespace hbm {
	namespace sys {


		EventLoop::EventLoop()
			: m_eventCount(0)
			, m_epollfd(epoll_create(1)) // parameter is ignored but must be greater than 0
			, m_stopFd(eventfd(0, EFD_NONBLOCK))
		{
			if (m_epollfd==-1) {
				throw hbm::exception::exception(std::string("epoll_create failed ") + strerror(errno));
			}

			struct epoll_event ev;
			memset(&ev, 0, sizeof(ev));
			ev.events = EPOLLIN | EPOLLET;
			ev.data.ptr = nullptr;
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
			{
				std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
				eventInfos_t::iterator eventsIter = m_eventInfos.find(fd);

				if (eventsIter==m_eventInfos.end()) {
					EventsHandlers_t eventHandlers;
					eventHandlers.inEvent = eventHandler;
					std::pair < eventInfos_t::iterator, bool > result = m_eventInfos.emplace(std::make_pair(fd, eventHandlers));
					eventsIter = result.first;
					mode = EPOLL_CTL_ADD;
				} else {
					// output event is already registered for this fd
					EventsHandlers_t &eventHandlers = eventsIter->second;
					if (eventHandlers.outEvent) {
						ev.events |= EPOLLOUT;
					}
					eventHandlers.inEvent = eventHandler;
				}
				ev.data.ptr = &eventsIter->second;


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
			{
				std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
				eventInfos_t::iterator eventsIter = m_eventInfos.find(fd);

				if (eventsIter==m_eventInfos.end()) {
					EventsHandlers_t eventHandlers;
					eventHandlers.outEvent = eventHandler;
					std::pair < eventInfos_t::iterator, bool > result = m_eventInfos.emplace(std::make_pair(fd, eventHandlers));
					eventsIter = result.first;
					mode = EPOLL_CTL_ADD;
				} else {
					// input event is already registered for this fd
					EventsHandlers_t &eventHandlers = eventsIter->second;
					if (eventHandlers.inEvent) {
						ev.events |= EPOLLIN;
					}
					eventHandlers.outEvent = eventHandler;
				}
				ev.data.ptr = &eventsIter->second;


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
			uint32_t removedEvents;
			eventInfos_t::iterator eventsIter = m_eventInfos.find(fd);
			if (eventsIter==m_eventInfos.end()) {
				return -1;
			}

			EventsHandlers_t &eventHandlers = eventsIter->second;
			if (eventHandlers.inEvent==nullptr) {
				return -1;
			}

			if (eventHandlers.outEvent) {
				// keep the existing EPOLLOUT event
				removedEvents = EPOLLIN;
				eventHandlers.inEvent = nullptr;
				struct epoll_event ev;
				memset(&ev, 0, sizeof(ev));
				ev.events = EPOLLOUT | EPOLLET;
				ev.data.ptr = &eventHandlers;
				ret = epoll_ctl(m_epollfd, EPOLL_CTL_MOD, fd, &ev);
			} else {
				removedEvents = EPOLLIN | EPOLLOUT;
				ret = epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, nullptr);
				m_eventInfos.erase(eventsIter);
			}

			for (int eventIndex = 0; eventIndex<m_eventCount; ++eventIndex) {
				if (m_events[eventIndex].data.ptr==&eventsIter->second) {
					// Turn all removed events off so that no handlers callback function will be called from
					// pending events afterwards!
					// This is important so that:
					// - entry can safely be erased from m_eventInfos
					// - callback methods can safely be removed.
					m_events[eventIndex].events &= ~removedEvents;
					break;
				}
			}

			return ret;
		}

		int EventLoop::eraseOutEvent(event fd)
		{
			std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
			int ret;
			uint32_t removedEvents;
			eventInfos_t::iterator eventsIter = m_eventInfos.find(fd);
			if (eventsIter==m_eventInfos.end()) {
				return -1;
			}
			EventsHandlers_t &eventHandlers = eventsIter->second;
			if (eventHandlers.outEvent==nullptr) {
				return -1;
			}
			
			if (eventHandlers.inEvent) {
				// keep the existing EPOLLIN event
				removedEvents = EPOLLOUT;
				eventHandlers.outEvent = nullptr;
				struct epoll_event ev;
				memset(&ev, 0, sizeof(ev));
				ev.events = EPOLLIN | EPOLLET;
				ev.data.ptr = &eventHandlers;
				ret = epoll_ctl(m_epollfd, EPOLL_CTL_MOD, fd, &ev);
			} else {
				removedEvents = EPOLLIN | EPOLLOUT;
				ret = epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, nullptr);
				m_eventInfos.erase(eventsIter);
			}

			for (int eventIndex = 0; eventIndex<m_eventCount; ++eventIndex) {
				if (m_events[eventIndex].data.ptr==&eventsIter->second) {
					// Turn all removed events off so that no handlers callback function will be called from
					// pending events afterwards!
					// This is important so that:
					// - entry can safely be erased from m_eventInfos
					// - callback methods can safely be removed.
					m_events[eventIndex].events &= ~removedEvents;
					break;
				}
			}

			return ret;
		}
		
		int EventLoop::execute()
		{
			ssize_t result;
			EventsHandlers_t *pEventHandlers;
			unsigned int eventsLeft;

			while (true) {
				m_eventCount = epoll_wait(m_epollfd, m_events, MAXEVENTS, -1);

				if (m_eventCount==-1) {
					if (errno!=EINTR) {
						// ignore interuption by signal
						return m_eventCount;
					}
				}

				{
					std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
					// We are working edge triggered, hence we need to process everything that is available for each event.
					// To be fair, the callback of each signaled event is called only once.
					// After all the callbacks of all signaled events were called, we start from the beginning until no signaled event is left.
					do {
						eventsLeft = 0;
						for (int n = 0; n < m_eventCount; ++n) {
							pEventHandlers = reinterpret_cast < EventsHandlers_t* > (m_events[n].data.ptr);
							if (pEventHandlers) {
								// we are working edge triggered, hence we need to read everything that is available
								if (m_events[n].events & EPOLLIN) {
									try {
										result = pEventHandlers->inEvent();
										if (result>0) {
											// there might be more to read...
											++eventsLeft;
										} else {
											// we are done with this event
											m_events[n].events &= ~EPOLLIN;
										}
									} catch (const std::exception& e) {
										syslog(LOG_ERR, "Event loop caught exception from input event callback method: '%s'", e.what());
									} catch (...) {
										syslog(LOG_ERR, "Event loop caught exception from input event callback method");
										// ignore
									}
								}
								if (m_events[n].events & EPOLLOUT) {
									try {
										result = pEventHandlers->outEvent();
										if (result>0) {
											// there might be more to write...
											++eventsLeft;
										} else {
											// we are done with this event
											m_events[n].events &= ~EPOLLOUT;
										}
									} catch (const std::exception& e) {
										syslog(LOG_ERR, "Event loop caught exception from output event callback method: '%s'", e.what());
									} catch (...) {
										syslog(LOG_ERR, "Event loop caught exception from output event callback method");
										// ignore
									}
								}
							} else {
								// We are working edge triggered. Reading away the event is not necessary.
								// Stop eventloop notification!
								return 0;
							}
						}
					} while (eventsLeft);
				}
			}
		}

		void EventLoop::stop()
		{
			if (write(m_stopFd, &notifyValue, sizeof(notifyValue))<0) {
				syslog(LOG_ERR, "notifying stop of eventloop failed");
			}
		}
	}
}
