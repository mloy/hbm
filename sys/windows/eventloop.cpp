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
		EventLoop::EventLoop()
			: m_completionPort(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1))
			, m_hEventLog(RegisterEventSource(NULL, reinterpret_cast < LPCSTR > ("Application")))
		{
		}

		EventLoop::~EventLoop()
		{
			stop();
			DeregisterEventSource(m_hEventLog);
		}

		int EventLoop::addEvent(event fd, EventHandler_t eventHandler)
		{
			if (!eventHandler) {
				return -1;
			}

			{
				std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
				m_eventInfos[fd.overlapped.hEvent] = eventHandler;
			}
			
			if (fd.fileHandle == INVALID_HANDLE_VALUE) {
				return 0;
			}

			if (CreateIoCompletionPort(fd.fileHandle, m_completionPort, 0, 1) == NULL) {
				int lastError = GetLastError();
				// ERROR_INVALID_PARAMETER means that this handle is already registered
				if (lastError != ERROR_INVALID_PARAMETER) {
					std::string message;
					LPCSTR messages;

					message = "Could not add event to event loop '" + std::to_string(lastError) + "'";
					messages = message.c_str();
					ReportEvent(m_hEventLog, EVENTLOG_ERROR_TYPE, 0, 0, NULL, 1, 0, reinterpret_cast < LPCSTR* > (&messages), NULL);
					return -1;
				}
			}
			return 0;

		}

		int EventLoop::eraseEvent(event fd)
		{
			std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
			m_eventInfos.erase(fd.overlapped.hEvent);
			return 0;
		}

		int EventLoop::execute()
		{
			BOOL result;
			DWORD size;
			ULONG_PTR completionKey;
			OVERLAPPED* pOverlapped;
			
			do {
				pOverlapped = NULL;
				result = GetQueuedCompletionStatus(m_completionPort, &size, &completionKey, &pOverlapped, INFINITE);
				if (result == FALSE) {
					int lastError = GetLastError();
					// ERROR_OPERATION_ABORTED happens on cancelation of an overlapped operation.
					// ERROR_NETNAME_DELETED happens on closure of connection
					if ((lastError != ERROR_OPERATION_ABORTED) && (lastError != ERROR_NETNAME_DELETED)) {
						std::string message;
						LPCSTR messages;

						message = "Event loop stopped with error " + std::to_string(lastError);
						messages = message.c_str();
						ReportEvent(m_hEventLog, EVENTLOG_ERROR_TYPE, 0, 0, NULL, 1, 0, reinterpret_cast < LPCSTR* > (&messages), NULL);
						break;
					}
				}
				if (pOverlapped == NULL) {
					// stop condition
					break;
				}

				{
					ssize_t result;

					std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
					eventInfos_t::iterator iter = m_eventInfos.find(pOverlapped->hEvent);
					if (iter != m_eventInfos.end()) {
						do {
							result = iter->second();
						} while (result > 0);
					} 
				}


			} while (true);
			return 0;

		}

		void EventLoop::stop()
		{
			PostQueuedCompletionStatus(m_completionPort, 0, 0, NULL);
		}
	}
}
