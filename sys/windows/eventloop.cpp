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


#include "hbm/sys/eventloop.h"

namespace hbm {
	namespace sys {
		/// \throws hbm::exception
		EventLoop::EventLoop()
		{

			m_stopEvent.fd = m_stopNotifier.getFd();
			m_stopEvent.eventHandler = nullptr;
			m_eventInfos.push_back(m_stopEvent);

			m_changeEvent.fd = m_changeNotifier.getFd();
			m_changeEvent.eventHandler = nullptr;
			m_eventInfos.push_back(m_changeEvent);
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

			m_changeNotifier.notify();
		}

		int EventLoop::execute()
		{
			return execute_for(std::chrono::milliseconds());
			//ssize_t nbytes = 0;

			//DWORD dwEvent;
			//eventInfo_t evi;
			//do {
			//	std::vector < HANDLE > handles;
			//	for (unsigned int i = 0; i < m_eventInfos.size(); ++i) {
			//		handles.push_back(m_eventInfos[i].fd);
			//	}

			//	do {
			//		dwEvent = WaitForMultipleObjects(handles.size(), &handles[0], FALSE, INFINITE);
			//		if ((dwEvent == WAIT_FAILED) || (dwEvent == WAIT_TIMEOUT)) {
			//			return -1;
			//		}

			//		evi = m_eventInfos[WAIT_OBJECT_0 + dwEvent];

			//		// this is a workaround because WSARecvMsg does not reset the event!
			//		WSAResetEvent(evi.fd);

			//		if (evi.eventHandler == nullptr) {
			//			// stop or change causes leaving the loop
			//			break;
			//		}
			//		do {
			//			// we do this until nothing is left. This is important because our call of WSARecvEvent above.
			//			nbytes = evi.eventHandler();
			//			if (nbytes < 0) {
			//				return nbytes;
			//			}
			//		} while (nbytes > 0);
			//	} while (nbytes >= 0);
			//} while (evi.fd == m_changeNotifier.getFd()); // in case of change we start all over again!

			//return 0;
		}

		int EventLoop::execute_for(std::chrono::milliseconds timeToWait)
		{
			DWORD timeout;
			ssize_t nbytes = 0;
			std::chrono::steady_clock::time_point endTime;
			endTime = std::chrono::steady_clock::now() + timeToWait;

			DWORD dwEvent;
			eventInfo_t evi;
			do {
				std::cout << "init" << std::endl;
				std::vector < HANDLE > handles;

				for (unsigned int i = 0; i < m_eventInfos.size(); ++i) {
					handles.push_back(m_eventInfos[i].fd);
				}

				do {
					if (endTime != std::chrono::steady_clock::time_point()) {
						std::chrono::milliseconds timediff = std::chrono::duration_cast <std::chrono::milliseconds> (endTime - std::chrono::steady_clock::now());

						timeout = static_cast<int> (timediff.count());
					}
					dwEvent = WaitForMultipleObjects(handles.size(), &handles[0], FALSE, timeout);
					if (dwEvent == WAIT_FAILED) {
						return -1;
						break;
					}

					if (dwEvent == WAIT_TIMEOUT) {
						return 0;
						break;
					}

					evi = m_eventInfos[WAIT_OBJECT_0 + dwEvent];
					if (evi.eventHandler == nullptr) {
						break;
					}

					nbytes = evi.eventHandler();
					if (nbytes < 0) {
						std::cout << "stopped!" << std::endl;

						return nbytes;
					}
				} while (nbytes >= 0);
			} while (evi.fd == m_changeNotifier.getFd()); // in case of change we start all over again!

			std::cout << "stopped!" << std::endl;


			return 0;
		}

		void EventLoop::stop()
		{
			m_stopNotifier.notify();
		}
	}
}
