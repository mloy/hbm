/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2007 Hottinger Baldwin Messtechnik GmbH
 * Im Tiefen See 45
 * 64293 Darmstadt
 * Germany
 * http://www.hbm.com
 * All rights reserved
 *
 * The copyright to the computer program(s) herein is the property of
 * Hottinger Baldwin Messtechnik GmbH (HBM), Germany. The program(s)
 * may be used and/or copied only with the written permission of HBM
 * or in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 * This copyright notice must not be removed.
 *
 * This Software is licenced by the
 * "General supply and license conditions for software"
 * which is part of the standard terms and conditions of sale from HBM.
*/
#include <cstring>

#include <sys/timerfd.h>
#include <unistd.h>

#include "hbm/exception/exception.hpp"

#include "hbm/sys/timer.h"

namespace hbm {
	namespace sys {
		Timer::Timer()
			: m_fd(timerfd_create(CLOCK_MONOTONIC, 0))
		{
			if (m_fd<0) {
				throw hbm::exception::exception("could not create timer fd");
			}
		}

		Timer::Timer(unsigned int period_ms)
			: m_fd(timerfd_create(CLOCK_MONOTONIC, 0))
		{
			if (m_fd<0) {
				throw hbm::exception::exception("could not create timer fd");
			}

			set(period_ms);
		}

		Timer::~Timer()
		{
			close(m_fd);
		}

		int Timer::set(unsigned int period_ms)
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
			timespec.it_interval.tv_sec = period_s;
			timespec.it_interval.tv_nsec = rest * 1000 * 1000;

			return timerfd_settime(m_fd, 0, &timespec, nullptr);
		}

		int Timer::wait()
		{
			struct itimerspec currValue;
			timerfd_gettime(m_fd, &currValue);
			if(currValue.it_value.tv_sec==0 && currValue.it_value.tv_nsec==0) {
				// not started!
				return -1;
			}

			uint64_t timerEventCount;
			if (read(m_fd, &timerEventCount, sizeof(timerEventCount))<0) {
				// timer was stopped!
				return 0;
			} else {
				// to be compatible between windows and linux, we return 1 even if timer expired timerEventCount times.
				return 1;
			}
		}

		int Timer::cancel()
		{
			return ::close(m_fd);
		}

		event Timer::getFd() const
		{
			return m_fd;
		}
	}
}
