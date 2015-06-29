// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <chrono>
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
			if(m_epollfd==-1) {
				throw hbm::exception::exception(std::string("epoll_create failed ") + strerror(errno));
			}

			struct epoll_event ev;
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
		}

		int EventLoop::addEvent(event fd, EventHandler_t eventHandler)
		{
			std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);

			if(!eventHandler) {
				return -1;
			}
			eventInfo_t item;
			item.fd = fd;
			item.eventHandler = eventHandler;

			m_eventInfos[item.fd] = item;

			struct epoll_event ev;
			ev.events = EPOLLIN | EPOLLET;
			// important: elements of maps are guaranteed to keep there position in memory if members are added/removed!
			ev.data.ptr = &m_eventInfos[item.fd];
			if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, item.fd, &ev) == -1) {
				syslog(LOG_ERR, "epoll_ctl failed while adding event '%s' epoll_d:%d, event_fd:%d", strerror(errno), m_epollfd, item.fd);
				return -1;
			}

			// there might have been work to do before fd was added to epoll. This won't be signaled by edge triggered epoll. Try until there is nothing left.
			ssize_t result;
			do {
				result = item.eventHandler();
			} while (result>0);
			return 0;
		}

		int EventLoop::eraseEvent(event fd)
		{
			std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);

			m_eventInfos.erase(fd);
			return epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, NULL);
		}


		int EventLoop::execute()
		{
			int nfds;
			struct epoll_event events[MAXEVENTS];

			while (true) {
				do {
					nfds = epoll_wait(m_epollfd, events, MAXEVENTS, -1);
				} while ((nfds==-1) && (errno==EINTR));

				if (nfds<=0) {
					// 0 means time out but is not possible here!
					syslog(LOG_ERR, "epoll_wait failed ('%s') in eventloop ", strerror(errno));
				}


				{
					std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
					for (int n = 0; n < nfds; ++n) {
						if(events[n].events & EPOLLIN) {
							eventInfo_t* pEventInfo = reinterpret_cast < eventInfo_t* > (events[n].data.ptr);
							if(pEventInfo==nullptr) {
								// stop notification!
								return 0;
							}
							ssize_t result;
							do {
								// we are working edge triggered, hence we need to read everything that is available
								result = pEventInfo->eventHandler();
							} while (result>0);
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

				if (nfds==0) {
					// time out!
					return nfds;
				} else if (nfds<0) {
					syslog(LOG_ERR, "epoll_wait failed ('%s') in eventloop ", strerror(errno));
				}


				{
					std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);

					for (int n = 0; n < nfds; ++n) {
						if(events[n].events & EPOLLIN) {
							eventInfo_t* pEventInfo = reinterpret_cast < eventInfo_t* > (events[n].data.ptr);
							if(pEventInfo==nullptr) {
								// stop notification!
								return 0;
							}
							ssize_t result;
							do {
								// we are working edge triggered, hence we need to read everything that is available
								result = pEventInfo->eventHandler();
							} while (result>0);
						}
					}
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
