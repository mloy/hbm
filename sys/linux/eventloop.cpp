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


#include <syslog.h>
#include <sys/epoll.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread_time.hpp>

#include "hbm/sys/eventloop.h"

namespace hbm {
	namespace sys {
		EventLoop::EventLoop()
			: m_epollfd(epoll_create(1)) // parameter is ignored but must be greater than 0
		{
			if(m_epollfd==-1) {
				throw hbm::exception::exception(std::string("epoll_create failed)") + strerror(errno));
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

			eventInfo_t& eviRef = m_eventInfos[fd] = evi;


			struct epoll_event ev;
			ev.events = EPOLLIN;
			ev.data.ptr = &eviRef;
			if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, eviRef.fd, &ev) == -1) {
				syslog(LOG_ERR, "epoll_ctl failed %s", strerror(errno));
			}
		}

		void EventLoop::eraseEvent(event fd)
		{
			eventInfos_t::iterator iter = m_eventInfos.find(fd);
			if(iter!=m_eventInfos.end()) {
				const eventInfo_t& eviRef = iter->second;
				epoll_ctl(m_epollfd, EPOLL_CTL_DEL, eviRef.fd, NULL);
				m_eventInfos.erase(iter);
			}
		}

		void EventLoop::clear()
		{
			for (eventInfos_t::iterator iter = m_eventInfos.begin(); iter!=m_eventInfos.end(); ++iter) {
				const eventInfo_t& eviRef = iter->second;
				epoll_ctl(m_epollfd, EPOLL_CTL_DEL, eviRef.fd, NULL);
			}
			m_eventInfos.clear();
		}

		int EventLoop::execute(boost::posix_time::milliseconds timeToWait)
		{
			int timeout = -1;
			ssize_t nbytes = 0;
			boost::posix_time::ptime endTime;
			if(timeToWait!=boost::posix_time::milliseconds(0)) {
				endTime = boost::get_system_time() + timeToWait;
			}

			int nfds;
			static const unsigned int MAXEVENTS = 16;
			struct epoll_event events[MAXEVENTS];

			do {
				if(endTime!=boost::posix_time::not_a_date_time) {
					boost::posix_time::time_duration timediff = endTime-boost::get_system_time();

					timeout =static_cast< int > (timediff.total_milliseconds());
					if(timeout<0) {
						break;
					}
				}
				nfds = epoll_wait(m_epollfd, events, MAXEVENTS, timeout);
				if((nfds==0) || (nfds==-1)) {
					// 0: time out!
					break;
				}

				for (int n = 0; n < nfds; ++n) {
					if(events[n].events & EPOLLIN) {
						eventInfo_t* pEventInfo = reinterpret_cast < eventInfo_t* > (events[n].data.ptr);
						nbytes = pEventInfo->eventHandler();
						if(nbytes<0) {
							break;
						}
					}
				}
			} while (nbytes>=0);
			return 0;
		}
	}
}
