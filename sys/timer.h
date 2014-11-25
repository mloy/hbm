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

#ifndef _HBM__TIMER_H
#define _HBM__TIMER_H

#ifdef _WIN32
#include <WinSock2.h>
#ifndef ssize_t
#define ssize_t int
#endif
typedef HANDLE event;
#else
#include <unistd.h>
typedef int event;
#endif

#include "hbm/exception/exception.hpp"

namespace hbm {
	namespace sys {
		/// a single-shot-timer. starts when setting the period. Event gets signaled when period is reached.
		class Timer {
		public:
			/// \throws hbm::exception
			Timer();
			/// \throws hbm::exception
			Timer(unsigned int period_s);
			~Timer();

			int set(unsigned int period_s);

			/// \return 0 if timer was stopped before expiration. 1 if timer has expired. -1 if timer was not started
			int wait();

			/// to poll
			event getFd() const;

			int cancel();

		private:
			/// must not be copied
			Timer(const Timer& op);
			/// must not be assigned
			Timer operator=(const Timer& op);

			event m_fd;
#ifdef _WIN32
			/// workaround for windows to determine whether the timer is stopped or got signaled
			bool m_canceled;
#endif

		};
	}
}
#endif
