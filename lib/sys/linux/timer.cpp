// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <syslog.h>
#include <cstring>
#include <inttypes.h>

#include <sys/timerfd.h>
#include <unistd.h>
#include <cstring> // for memset()


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
			// We do not really need to know the count. Read gives 8 byte if timer triggered at least once.
			uint64_t timerEventCount = 0;
			// it is sufficient to read once in order to rearm cyclic timers
			ssize_t result = ::read(m_fd, &timerEventCount, sizeof(timerEventCount));
			if (static_cast < size_t > (result)==sizeof(timerEventCount)) {
				if (m_eventHandler) {
						m_eventHandler(true);
				}
			}
			return 0;
		}
	}
}
