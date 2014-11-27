// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#include <WinSock2.h>
#include <Windows.h>
#define syslog fprintf
#define LOG_DEBUG stdout
#define LOG_INFO stdout
#define LOG_ERR stderr
#define ssize_t int


#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread_time.hpp>

#include "hbm/sys/eventloop.h"

namespace hbm {
	namespace sys {
		/// \throws hbm::exception
		EventLoop::EventLoop()
		{
		}

		EventLoop::~EventLoop()
		{		
		}

		void EventLoop::addEvent(event fd, eventHandler_t eventHandler)
		{
			eventInfo_t evi;
			evi.fd = fd;
			evi.eventHandler = eventHandler;

			m_eventInfos.push_back(evi);
		}

		int EventLoop::execute(boost::posix_time::milliseconds timeToWait)
		{
			if(m_eventInfos.empty()) {
				return -1;
			}

			int timeout = -1;
			ssize_t nbytes = 0;
			boost::posix_time::ptime endTime;
			if(timeToWait!=boost::posix_time::milliseconds(0)) {
				endTime = boost::get_system_time() + timeToWait;
			}

			DWORD dwEvent;
			std::vector < HANDLE > handles;

			//for(eventInfos_t::iterator iter=m_eventInfos.begin(); iter!=m_eventInfos.end(); ++iter) {
			for(unsigned int i=0; i<m_eventInfos.size(); ++i) {
				handles.push_back(m_eventInfos[i].fd);
			}

			do {
				if(endTime!=boost::posix_time::not_a_date_time) {
					boost::posix_time::time_duration timediff = endTime-boost::get_system_time();

					timeout =static_cast< int > (timediff.total_milliseconds());
					if(timeout<0) {
						break;
					}
				}
				dwEvent = WaitForMultipleObjects(handles.size(), &handles[0], FALSE, timeout);
				if( (dwEvent==WAIT_FAILED) || (dwEvent==WAIT_TIMEOUT) ) {
					nbytes = -1;
					break;
				}

				eventInfo_t& evi = m_eventInfos[WAIT_OBJECT_0 + dwEvent];
				nbytes = evi.eventHandler();
				if(nbytes<0) {
					break;
				}
			} while (nbytes>=0);
			return 0;
		}
	}
}
