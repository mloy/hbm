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

#include "hbm/system/timer.h"

namespace hbm {
	Timer::Timer()
		: m_fd(timerfd_create(CLOCK_MONOTONIC, 0))
	{
		if (m_fd<0) {
			throw hbm::exception::exception("could not create timer fd");
		}
	}

	Timer::Timer(unsigned int period_s)
		: m_fd(timerfd_create(CLOCK_MONOTONIC, 0))
	{
		if (m_fd<0) {
			throw hbm::exception::exception("could not create timer fd");
		}

		set(period_s);
	}

	Timer::~Timer()
	{
		close(m_fd);
	}

	void Timer::set(unsigned int period_s)
	{
		struct itimerspec timespec;
		memset (&timespec, 0, sizeof(timespec));
		timespec.it_value.tv_sec = period_s;
		timespec.it_interval.tv_sec = period_s;

		timerfd_settime(m_fd, 0, &timespec, nullptr);
	}

	ssize_t Timer::receive()
	{
		uint64_t timerEventCount;
		return read(m_fd, &timerEventCount, sizeof(timerEventCount));
	}

	int Timer::stop()
	{
		return ::close(m_fd);
	}

	event Timer::getFd() const
	{
		return m_fd;
	}
}
