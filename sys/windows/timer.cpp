// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#include <iostream>
#include <WinSock2.h>

#include <cstring>
#include <stdint.h>


#include "hbm/sys/timer.h"

namespace hbm {
	namespace sys {
		Timer::Timer()
			: m_fd(NULL)
		{
			m_fd = CreateWaitableTimer(NULL, FALSE, NULL);
			cancel();
		}

		Timer::Timer(unsigned int period_ms, bool repeated)
			: m_fd(NULL)
		{
			m_fd = CreateWaitableTimer(NULL, FALSE, NULL);

			set(period_ms, repeated);
		}

		Timer::~Timer()
		{
			CloseHandle(m_fd);
		}

		int Timer::set(unsigned int period_ms, bool repeated)
		{
			LARGE_INTEGER dueTime;
			static const int64_t multilpier = -10000; // negative because we want a relative time
			LONG period = 0; // in ms

			if (repeated) {
				period = period_ms;
			}
			dueTime.QuadPart = period_ms*multilpier; // in 100ns
			BOOL Result = SetWaitableTimer(
				m_fd,
				&dueTime,
				period,
				NULL,
				NULL,
				FALSE
				);
			if(Result==0) {
				return -1;
			}
			return 0;
		}

		int Timer::read()
		{
			DWORD result = WaitForSingleObject(m_fd, 0);
			switch (result) {
			case WAIT_OBJECT_0:
				return 1;
				break;
			case WAIT_TIMEOUT:
				return 0;
				break;
			default:
				return -1;
				break;
			}
			return 0;
		}

		int Timer::wait()
		{
			return wait_for(INFINITE);
		}


		int Timer::wait_for(int period_ms)
		{
			DWORD result = WaitForSingleObject(m_fd, period_ms);
			switch (result) {
			case WAIT_OBJECT_0:
				return 1;
				break;
			default:
				return -1;
				break;
			}
			return 0;
		}

		int Timer::cancel()
		{
			LARGE_INTEGER dueTime;
			dueTime.QuadPart = LLONG_MIN;
			BOOL Result = SetWaitableTimer(
				m_fd,
				&dueTime,
				0,
				NULL,
				NULL,
				FALSE
				);
			return 0;
		}

		event Timer::getFd() const
		{
			return m_fd;
		}
	}
}
