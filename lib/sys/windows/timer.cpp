// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#include <iostream>
#include <WinSock2.h>

#include <cstring>
#include <stdint.h>


#include "hbm/sys/timer.h"


static void CALLBACK timerCb(void *pData, BOOLEAN fired)
{
	if (fired) {
		hbm::sys::event* pEvent = reinterpret_cast <hbm::sys::event*> (pData);
		PostQueuedCompletionStatus(pEvent->completionPort, 0, 0, &pEvent->overlapped);
	}
}


namespace hbm {
	namespace sys {
		Timer::Timer(EventLoop& eventLoop)
			: m_fd()
			, m_eventLoop(eventLoop)
			, m_eventHandler()
		{
			m_fd.completionPort = m_eventLoop.getCompletionPort();
		}

		Timer::~Timer()
		{
			m_eventLoop.eraseEvent(m_fd);
			if (m_fd.overlapped.hEvent) {
				DeleteTimerQueueTimer(NULL, m_fd.overlapped.hEvent, NULL);
			}
		}

		int Timer::set(std::chrono::milliseconds period, bool repeated, Cb_t eventHandler)
		{
			return set(static_cast <unsigned int>(period.count()), repeated, eventHandler);
		}

		int Timer::set(unsigned int period_ms, bool repeated, Cb_t eventHandler)
		{
			m_eventLoop.eraseEvent(m_fd);

			if (m_fd.overlapped.hEvent) {
				DeleteTimerQueueTimer(NULL, m_fd.overlapped.hEvent, NULL);
				m_fd.overlapped.hEvent = NULL;
			}
			//cancel();

			DWORD repeatPeriod;
			if (repeated) {
				repeatPeriod = period_ms;
			}
			else {
				repeatPeriod = 0;
			}

			m_eventHandler = eventHandler;

			if (CreateTimerQueueTimer(&m_fd.overlapped.hEvent, NULL, &timerCb, &m_fd, period_ms, repeatPeriod, WT_EXECUTEINTIMERTHREAD)) {
				return m_eventLoop.addEvent(m_fd, std::bind(&Timer::process, std::ref(*this)));
			} else {
				return -1;
			}
		}

		int Timer::process()
		{
			if (m_eventHandler) {
				m_eventHandler(true);
			}
			return 0;
		}


		int Timer::cancel()
		{
			m_eventLoop.eraseEvent(m_fd);

			if (m_fd.overlapped.hEvent) {
				DeleteTimerQueueTimer(NULL, m_fd.overlapped.hEvent, NULL);
				m_fd.overlapped.hEvent = NULL;
			}
			if (m_eventHandler) {
				m_eventHandler(false);
			}
			return 0;
		}
	}
}
