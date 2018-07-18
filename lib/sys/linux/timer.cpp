// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <syslog.h>
#include <cstring>
#include <inttypes.h>

#include <sys/timerfd.h>
#include <unistd.h>
#include <string.h> // for memset()


#include "hbm/exception/exception.hpp"

#include "hbm/sys/timer.h"

namespace hbm {
	namespace sys {
		Timer::Timer(EventLoop &eventLoop)
			: m_fd(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK))
			, m_eventLoop(eventLoop)
			, m_eventHandler()
		{
			if (m_fd<0) {
				throw hbm::exception::exception(std::string("could not create timer fd '") + strerror(errno) + "'");
			}
			if (m_eventLoop.addEvent(m_fd, std::bind(&Timer::process, this))<0) {
				throw hbm::exception::exception("could not add timer to event loop");
			}
		}

		Timer::~Timer()
		{
			m_eventLoop.eraseEvent(m_fd);
			cancel();
			close(m_fd);
		}

		int Timer::set(std::chrono::milliseconds period, bool repeated, Cb_t eventHandler)
		{
			return set(period.count(), repeated, eventHandler);
		}

		int Timer::set(unsigned int period_ms, bool repeated, Cb_t eventHandler)
		{
			if (period_ms==0) {
				return -1;
			}

			struct itimerspec timespec;
			memset (&timespec, 0, sizeof(timespec));
			unsigned int period_s = period_ms / 1000;
			unsigned int rest = period_ms % 1000;

			timespec.it_value.tv_sec = period_s;
			timespec.it_value.tv_nsec = rest * 1000 * 1000;
			if (repeated) {
				timespec.it_interval.tv_sec = period_s;
				timespec.it_interval.tv_nsec = rest * 1000 * 1000;
			}
			m_eventHandler = eventHandler;
			return timerfd_settime(m_fd, 0, &timespec, nullptr);
		}

		int Timer::cancel()
		{
			int retval = 0;
			struct itimerspec timespec;

			// Before calling callback function with fired=false, we need to clear the callback routine. Otherwise a recursive call might happen
			Cb_t originalEventHandler = m_eventHandler;
			m_eventHandler = Cb_t();

			if (timerfd_gettime(m_fd, &timespec)==-1) {
				syslog(LOG_ERR, "error getting remaining time of timer %d '%s'", m_fd, strerror(errno));
				return -1;
			}
			if ( (timespec.it_value.tv_sec != 0) || (timespec.it_value.tv_nsec != 0) ) {
				// timer is running
				if (originalEventHandler) {
					originalEventHandler(false);
				}
				retval = 1;
			}

			memset (&timespec, 0, sizeof(timespec));
			timerfd_settime(m_fd, 0, &timespec, nullptr);

			return retval;
		}

		int Timer::process()
		{
			// callback function is to be executed once! We read until would block, execute once and return 0 so that the eventloop won't call again
			uint64_t timerEventCountSum = 0;
			uint64_t timerEventCount = 0;
			while (::read(m_fd, &timerEventCount, sizeof(timerEventCount))==sizeof(timerEventCount)) {
				if (timerEventCount>0) {
					timerEventCountSum += timerEventCount;
				}
			}

			if (timerEventCountSum) {
				if (timerEventCountSum>1) {
					// this is possible for cyclic timers only!
					syslog(LOG_WARNING, "cyclic timer %d elapsed %" PRIu64 " times before callback was executed.", m_fd, timerEventCountSum);
				}
				if (m_eventHandler) {
					m_eventHandler(true);
				}
			}
			return 0;
		}
	}
}
