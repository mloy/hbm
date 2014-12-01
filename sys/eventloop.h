// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#ifndef _EventLoop_H
#define _EventLoop_H


#ifdef _WIN32
	#include <vector>
	#include <WinSock2.h>
	#include <Windows.h>
	typedef HANDLE event;
#else
	#include <unordered_map>
	typedef int event;
#endif
#include <functional>

#if _MSC_VER && _MSC_VER < 1700
#include <boost/date_time/posix_time/posix_time_types.hpp>
#else
#include <chrono>
#endif
#include "hbm/exception/exception.hpp"

namespace hbm {
	namespace sys {
		typedef std::function < int () > eventHandler_t;


		/// \warning not thread-safe
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
#if _MSC_VER && _MSC_VER < 1700
			int execute_for(boost::posix_time::milliseconds timeToWait);
#else
			int execute_for(std::chrono::milliseconds timeToWait);
#endif
		private:
			struct eventInfo_t {
				event fd;
				eventHandler_t eventHandler;
			};

			/// fd is the key
	#ifdef _WIN32
			typedef std::vector < eventInfo_t > eventInfos_t;
	#else
			typedef std::unordered_map <event, eventInfo_t > eventInfos_t;
			int m_epollfd;
	#endif

			eventInfos_t m_eventInfos;
		};
	}
}
#endif
