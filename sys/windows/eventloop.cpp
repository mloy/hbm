// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#define ssize_t int
#include <chrono>
#include <mutex>


#include "hbm/sys/eventloop.h"

namespace hbm {
	namespace sys {
		/// \throws hbm::exception
		EventLoop::EventLoop()
			: m_stopFd(CreateEvent(NULL, false, false, NULL))
		{
			eventInfo_t stopEvent;
			stopEvent.fd = m_stopFd;
			stopEvent.eventHandler = nullptr;


			m_eventInfos[m_stopFd] = stopEvent;

			m_handles.push_back(m_stopFd);
		}

		EventLoop::~EventLoop()
		{
			stop();
		}

		int EventLoop::addEvent(event fd, EventHandler_t eventHandler)
		{
			std::lock_guard < std::recursive_mutex> lock(m_eventInfosMtx);

			if (!eventHandler) {
				return -1;
			}

			eventInfo_t evi;
			evi.fd = fd;
			evi.eventHandler = eventHandler;
			m_eventInfos[fd] = evi;

			m_handles.clear();
			for (eventInfos_t::const_iterator iter = m_eventInfos.begin(); iter != m_eventInfos.end(); ++iter) {
				m_handles.push_back(iter->first);
			}
			return 0;
		}

		int EventLoop::eraseEvent(event fd)
		{
			std::lock_guard < std::recursive_mutex> lock(m_eventInfosMtx);
			m_eventInfos.erase(fd);

			m_handles.clear();
			for (eventInfos_t::const_iterator iter = m_eventInfos.begin(); iter != m_eventInfos.end(); ++iter) {
				m_handles.push_back(iter->first);
			}
			return 0;
		}

		int EventLoop::execute()
		{
			return execute_for(std::chrono::milliseconds());
		}

		int EventLoop::execute_for(std::chrono::milliseconds timeToWait)
		{
			DWORD timeout = INFINITE;
			ssize_t nbytes = 0;
			std::chrono::steady_clock::time_point endTime;
			if (timeToWait != std::chrono::milliseconds(0)) {
				endTime = std::chrono::steady_clock::now() + timeToWait;
			}

			DWORD dwEvent;
			eventInfo_t evi;
			do {
					
				if (endTime != std::chrono::steady_clock::time_point()) {
					std::chrono::milliseconds timediff = std::chrono::duration_cast <std::chrono::milliseconds> (endTime - std::chrono::steady_clock::now());
					if (timediff.count() > 0) {
						timeout = static_cast<int> (timediff.count());
					} else {
						timeout = 0;
					}
				}
				dwEvent = WaitForMultipleObjects(static_cast <DWORD> (m_handles.size()), &m_handles[0], FALSE, timeout);

				{
					std::lock_guard < std::recursive_mutex> lock(m_eventInfosMtx);
					if (dwEvent == WAIT_FAILED) {
						int lastError = GetLastError();
						// ERROR_INVALID_HANDLE might happen on removal of events.
						if (lastError != ERROR_INVALID_HANDLE) {
							return -1;
						}
						//changeHandler();
					}
					else if (dwEvent == WAIT_TIMEOUT) {
						// stop because of timeout
						return 0;
					}
					else {
						event fd = m_handles[WAIT_OBJECT_0 + dwEvent];
						evi = m_eventInfos[fd];
						// this is a workaround. WSARecvMsg does not reset the event!
						WSAResetEvent(fd);

						if (evi.eventHandler == nullptr) {
							break;
						}

						do {
							// we do this until nothing is left. This is important because of our call to WSAResetEvent above.
							nbytes = evi.eventHandler();
						} while (nbytes > 0);
					}
				}

			} while (true);


			return 0;
		}

		void EventLoop::stop()
		{
			SetEvent(m_stopFd);
		}
	}
}
