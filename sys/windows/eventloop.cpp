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
#include <chrono>


#include "hbm/sys/eventloop.h"

namespace hbm {
	namespace sys {
		/// \throws hbm::exception
		EventLoop::EventLoop()
		{
			m_stopEvent.fd = m_stopNotifier.getFd();
			m_stopEvent.eventHandler = nullptr;
			m_eventInfos.push_back(m_stopEvent);
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


		int EventLoop::execute()
		{
			ssize_t nbytes = 0;

			DWORD dwEvent;
			std::vector < HANDLE > handles;
			for (unsigned int i = 0; i<m_eventInfos.size(); ++i) {
				handles.push_back(m_eventInfos[i].fd);
			}

			do {
				dwEvent = WaitForMultipleObjects(handles.size(), &handles[0], FALSE, INFINITE);
				if ((dwEvent == WAIT_FAILED) || (dwEvent == WAIT_TIMEOUT)) {
					return -1;
				}

				eventInfo_t& evi = m_eventInfos[WAIT_OBJECT_0 + dwEvent];
				if (evi.eventHandler == nullptr) {
					return 0;
				}
				nbytes = evi.eventHandler();
				if (nbytes<0) {
					return nbytes;
				}
			} while (nbytes >= 0);
			return 0;
		}

		int EventLoop::execute_for(std::chrono::milliseconds timeToWait)
		{
			DWORD timeout;
			ssize_t nbytes = 0;
			std::chrono::steady_clock::time_point endTime;
			endTime = std::chrono::steady_clock::now() + timeToWait;

			DWORD dwEvent;
			std::vector < HANDLE > handles;

			for(unsigned int i=0; i<m_eventInfos.size(); ++i) {
				handles.push_back(m_eventInfos[i].fd);
			}

			do {
				if (endTime != std::chrono::steady_clock::time_point()) {
					std::chrono::milliseconds timediff = std::chrono::duration_cast < std::chrono::milliseconds > (endTime - std::chrono::steady_clock::now());

					timeout =static_cast< int > (timediff.count());
				}
				dwEvent = WaitForMultipleObjects(handles.size(), &handles[0], FALSE, timeout);
				if( (dwEvent==WAIT_FAILED) || (dwEvent==WAIT_TIMEOUT) ) {
					return -1;
					break;
				}

				eventInfo_t& evi = m_eventInfos[WAIT_OBJECT_0 + dwEvent];
				if (evi.eventHandler == nullptr) {
					return 0;
				}

				nbytes = evi.eventHandler();
				if(nbytes<0) {
					return nbytes;
				}
			} while (nbytes>=0);
			return 0;
		}

		void EventLoop::stop()
		{
			m_stopNotifier.notify();
		}
	}
}
