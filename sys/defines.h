// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#ifndef _HBM__SYS_DEFINES_H
#define _HBM__SYS_DEFINES_H

#ifdef _WIN32
#include <WinSock2.h>
#ifndef ssize_t
#define ssize_t int
#endif
#else
#include <unistd.h>
#endif

#include <functional>

namespace hbm {
	namespace sys {

#ifdef _WIN32
		struct event {
			event()
				: completionPort(INVALID_HANDLE_VALUE)
				, fileHandle(INVALID_HANDLE_VALUE)
			{
				memset(&overlapped, 0, sizeof(overlapped));
			}
			/// the io completion port used by the event loop
			HANDLE completionPort;
			OVERLAPPED overlapped;
			HANDLE fileHandle;
		};
#else
		typedef int event;
#endif
		typedef std::function < int () > EventHandler_t;
	}
}
#endif
