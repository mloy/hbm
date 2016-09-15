// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#ifndef _EventLoop_H
#define _EventLoop_H



#include <unordered_map>
#ifdef _WIN32
	#include <WinSock2.h>
	#include <Windows.h>
#endif
#include <functional>
#include <mutex>
#include <thread>

#include "hbm/exception/exception.hpp"
#include "hbm/sys/defines.h"

namespace hbm {
	namespace sys {
		/// The event loop is not responsible for handling errors returned by any callback routine. Error handling is to be done by the callback routine itself.
		class EventLoop {
		public:
			/// \throws hbm::exception
			EventLoop();
			virtual ~EventLoop();

			/// existing event handler of an fd will be replaced
			/// \param fd a non-blocking file descriptor to observe
			/// \param EventHandler_t callback function to be called if file descriptor gets signaled.
			int addEvent(event fd, EventHandler_t eventHandler);

			int eraseEvent(event fd);

			/// \return 0 stopped; -1 error
			int execute();

			void stop();

			std::thread::id getThreadId()
			{
				return std::this_thread::get_id();
			}

#ifdef _WIN32
			HANDLE getCompletionPort() const
			{
				return m_completionPort;
			}
#endif

		private:
#ifdef _WIN32
			typedef std::unordered_map <HANDLE, EventHandler_t > eventInfos_t;
			HANDLE m_completionPort;
			HANDLE m_hEventLog;
#else
			typedef std::unordered_map <event, EventHandler_t > eventInfos_t;
			int m_epollfd;
			event m_stopFd;
#endif
			/// events handled by event loop
			eventInfos_t m_eventInfos;
			std::recursive_mutex m_eventInfosMtx;
		};
	}
}
#endif
