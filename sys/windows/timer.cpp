// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#include <iostream>
#include <cstring>
#include <stdint.h>

#include <WinSock2.h>

#include "hbm/sys/timer.h"

namespace hbm {
	namespace sys {
		Timer::Timer()
			: m_fd(NULL)
			, m_canceled(false)
		{
		}

		Timer::Timer(unsigned int period_ms)
			: m_fd(NULL)
			, m_canceled(false)
		{
			set(period_ms);
		}

		Timer::~Timer()
		{
			CloseHandle(m_fd);
		}

		int Timer::set(unsigned int period_ms)
		{
			m_fd = CreateWaitableTimer(NULL, FALSE, NULL);
			LARGE_INTEGER dueTime;
			static const uint64_t multilpier = -10000; // negative because we want a relative time

			dueTime.QuadPart = period_ms*multilpier; // in 100ns
			BOOL Result = SetWaitableTimer(
				m_fd,
				&dueTime,
				period_ms, // in ms
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
				if (m_canceled) {
					return 0;
				}
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
			if (m_canceled) {
				return 0;
			}

			DWORD result = WaitForSingleObject(m_fd, INFINITE);
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
			m_canceled = true;
			CancelWaitableTimer(m_fd);
			return 0;
		}

		event Timer::getFd() const
		{
			return m_fd;
		}
	}
}
