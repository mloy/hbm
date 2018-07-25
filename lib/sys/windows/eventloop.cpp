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

#ifdef UNICODE
#define LPSTRINGTYPE LPCWSTR
#else
#define LPSTRINGTYPE LPCSTR
#endif
namespace hbm {
	namespace sys {
		EventLoop::EventLoop()
			: m_completionPort(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1))
			, m_hEventLog(RegisterEventSource(NULL, reinterpret_cast < LPSTRINGTYPE > ("Application")))
		{
		}

		EventLoop::~EventLoop()
		{
			stop();
			DeregisterEventSource(m_hEventLog);
			CloseHandle(m_completionPort);
		}

		int EventLoop::addEvent(event fd, const EventHandler_t &eventHandler)
		{
			if (!eventHandler) {
				return -1;
			}

			{
				std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
				m_inEventInfos[fd.overlapped.hEvent] = eventHandler;
			}
			
			if (fd.fileHandle == INVALID_HANDLE_VALUE) {
				return 0;
			}

			// We use the completionkey to determine the event to handle.
			// Using the ovelapped structure is dangearous because it is owned by the object that registers the event.
			// After destruction, GetQueuedCompletionStatus() might deliver an overlapped structure that does not exist anymore!
			ULONG completionKey = (ULONG)fd.overlapped.hEvent;
			if (CreateIoCompletionPort(fd.fileHandle, m_completionPort, (ULONG_PTR)completionKey, 1) == NULL) {
				int lastError = GetLastError();
				// ERROR_INVALID_PARAMETER means that this handle is already registered
				if (lastError != ERROR_INVALID_PARAMETER) {
					std::string message;
					LPCSTR messages;

					message = "Could not add event to event loop '" + std::to_string(lastError) + "'";
					messages = message.c_str();
					ReportEvent(m_hEventLog, EVENTLOG_ERROR_TYPE, 0, 0, NULL, 1, 0, reinterpret_cast < LPSTRINGTYPE* > (&messages), NULL);
					return -1;
				}
			}
			return 0;

		}

		int EventLoop::eraseEvent(event fd)
		{
			std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
			m_inEventInfos.erase(fd.overlapped.hEvent);
			return 0;
		}

		int EventLoop::execute()
		{
			BOOL result;
			DWORD size;
			ULONG_PTR completionKey = 0;
			OVERLAPPED* pOverlapped;


			if (m_completionPort == NULL) {
				LPCSTR messages = "Could not create io completion port!";
				ReportEvent(m_hEventLog, EVENTLOG_ERROR_TYPE, 0, 0, NULL, 1, 0, reinterpret_cast < LPSTRINGTYPE* > (&messages), NULL);
				return -1;
			}
			
			do {
				pOverlapped = NULL;
				result = GetQueuedCompletionStatus(m_completionPort, &size, &completionKey, &pOverlapped, INFINITE);
				if (result == FALSE) {
					if (pOverlapped==NULL) {
						// nothing was dequeued...
						LPCSTR messages = "Event loop woke up but nothing was dequeued. Waiting for next events...";
						ReportEvent(m_hEventLog, EVENTLOG_ERROR_TYPE, 0, 0, NULL, 1, 0, reinterpret_cast < LPSTRINGTYPE* > (&messages), NULL);
					} else {
						int lastError = GetLastError();
						
						if (lastError == ERROR_NETNAME_DELETED) {
							HANDLE event = (HANDLE)completionKey;
							//HANDLE event = pOverlapped->hEvent;

							// ERROR_NETNAME_DELETED happens on closure of connection
							// Happpens also if tcp keep alive recognizes loss of connection.
							// We need to call the callback routine only once to handle the error.
							std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
							eventInfos_t::iterator iter = m_inEventInfos.find(event);
							if (iter != m_inEventInfos.end()) {
								iter->second();
							}
						} else if (lastError != ERROR_OPERATION_ABORTED) {
							// ERROR_OPERATION_ABORTED happens on cancelation of an overlapped operation and is to be ignored.
							std::string message;
							LPCSTR messages;
	
							message = "Event loop stopped with error " + std::to_string(lastError);
							messages = message.c_str();
							ReportEvent(m_hEventLog, EVENTLOG_ERROR_TYPE, 0, 0, NULL, 1, 0, reinterpret_cast < LPSTRINGTYPE* > (&messages), NULL);
							break;
						}
					}
				} else {
//					if (pOverlapped==NULL) {
//						// stop condition
//						break;
//					}

//					{
					ssize_t retval = 0;
					HANDLE event = (HANDLE)completionKey;
					//HANDLE event = pOverlapped->hEvent;

					if (event == WSA_INVALID_EVENT) {
						// stop condition
						return 0;
					}

					do {
						std::lock_guard < std::recursive_mutex > lock(m_eventInfosMtx);
						eventInfos_t::iterator iter = m_inEventInfos.find(event);
						if (iter != m_inEventInfos.end()) {
							retval = iter->second();
						}
					} while (retval > 0);
//					}
				}
			} while (true);
			return 0;
		}

		void EventLoop::stop()
		{
			PostQueuedCompletionStatus(m_completionPort, 0, (ULONG_PTR)WSA_INVALID_EVENT, NULL);
		}
	}
}
