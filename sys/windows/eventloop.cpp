// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#define syslog fprintf
#define LOG_DEBUG stdout
#define LOG_INFO stdout
#define LOG_ERR stderr
#define ssize_t int
#include <chrono>
#include <mutex>


#include "hbm/sys/eventloop.h"

namespace hbm {
	namespace sys {
		/// \throws hbm::exception
		EventLoop::EventLoop()
		{
			init();
		}

		EventLoop::~EventLoop()
		{	
		}

		void EventLoop::init()
		{
			m_stopEvent.fd = m_stopNotifier.getFd();
			m_stopEvent.eventHandler = nullptr;

			m_changeEvent.fd = m_changeNotifier.getFd();
			m_changeEvent.eventHandler = nullptr;// std::bind(&EventLoop::changeHandler, this);;

			m_eventInfos[m_stopEvent.fd] = m_stopEvent;
			m_eventInfos[m_changeEvent.fd] = m_changeEvent;
		}

		int EventLoop::changeHandler()
		{
			return 0;
		}


		void EventLoop::addEvent(event fd, eventHandler_t eventHandler)
		{
			if (!eventHandler) {
				return;
			}

			eventInfo_t evi;
			evi.fd = fd;
			evi.eventHandler = eventHandler;
			{
				std::lock_guard < std::mutex> lock(m_changeListMtx);
				m_changeList.push_back(evi);
			}
			m_changeNotifier.notify();
		}

		void EventLoop::eraseEvent(event fd)
		{
			eventInfo_t evi;
			evi.fd = fd;
			evi.eventHandler = eventHandler_t();
			{
				std::lock_guard < std::mutex> lock(m_changeListMtx);
				m_changeList.push_back(evi);
			}
			m_changeNotifier.notify();
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
				std::vector < HANDLE > handles;

				{
					std::lock_guard < std::mutex > lock(m_changeListMtx);
					for (changelist_t::const_iterator iter = m_changeList.begin(); iter != m_changeList.end(); ++iter) {
						const eventInfo_t& item = *iter;
						if (item.eventHandler) {
							// add
							m_eventInfos[item.fd] = item;
						}
						else {
							// remove
							m_eventInfos.erase(item.fd);
						}
					}
					m_changeList.clear();
				}


				for (eventInfos_t::const_iterator iter = m_eventInfos.begin(); iter != m_eventInfos.end(); ++iter) {
					handles.push_back(iter->first);
				}

				do {
					if (endTime != std::chrono::steady_clock::time_point()) {
						std::chrono::milliseconds timediff = std::chrono::duration_cast <std::chrono::milliseconds> (endTime - std::chrono::steady_clock::now());

						timeout = static_cast<int> (timediff.count());
					}
					dwEvent = WaitForMultipleObjects(handles.size(), &handles[0], FALSE, timeout);
					if (dwEvent == WAIT_FAILED) {
						int retval = GetLastError();
						std::cout << retval << std::endl;
						return 0;
						break;
					}

					if (dwEvent == WAIT_TIMEOUT) {
						// stop because of timeout
						return 0;
						break;
					}

					event fd = handles[WAIT_OBJECT_0 + dwEvent];
					evi = m_eventInfos[fd];
					// this is a workaround. WSARecvMsg does not reset the event!
					WSAResetEvent(evi.fd);

					if (evi.eventHandler == nullptr) {
						break;
					}

					do {
						// we do this until nothing is left. This is important because of our call to WSAResetEvent above.
						nbytes = evi.eventHandler();
					} while (nbytes>0); 
					if (nbytes < 0) {
						// stop because of error
						return nbytes;
					}
				} while (nbytes >= 0);
			} while (evi.fd == m_changeNotifier.getFd()); // in case of change we start all over again!


			return 0;
		}

		void EventLoop::stop()
		{
			m_stopNotifier.notify();
		}
	}
}
