// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#ifndef _EventLoop_H
#define _EventLoop_H


#include <list>
#include <unordered_map>
#ifdef _WIN32
	#include <WinSock2.h>
	#include <Windows.h>
	typedef HANDLE event;
#else
	typedef int event;
#endif
#include <functional>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <mutex>

#include "hbm/exception/exception.hpp"
#include "hbm/sys/notifier.h"

namespace hbm {
	namespace sys {
		typedef std::function < int () > eventHandler_t;

		class EventLoop {
		public:
			/// \throws hbm::exception
			EventLoop();
			virtual ~EventLoop();
			void addEvent(event fd, eventHandler_t eventHandler);

			void eraseEvent(event fd);

			void clear();

			/// \return -1 eventloop stopped because one callback function returned error (-1).
			int execute();
			/// \return 0 if given time to wait was reached. -1 eventloop stopped because one callback function returned error (-1).
			int execute_for(std::chrono::milliseconds timeToWait);

			void stop();
		private:
			struct eventInfo_t {
				event fd;
				eventHandler_t eventHandler;
			};

			/// fd is the key
			typedef std::unordered_map <event, eventInfo_t > eventInfos_t;
			typedef std::list < eventInfo_t > changelist_t;

			/// called from within the event loop for thread-safe add and remove of events
			int changeHandler();
#ifdef _WIN32
			void init();
#else
			int m_epollfd;
#endif

			Notifier m_changeNotifier;
			Notifier m_stopNotifier;

			eventInfo_t m_stopEvent;
			eventInfo_t m_changeEvent;

			/// events to be added/removed go in here
			changelist_t m_changeList;
			std::mutex m_changeListMtx;

			/// events handeled by event loop
			eventInfos_t m_eventInfos;
		};
	}
}
#endif
