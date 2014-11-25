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
//#include <iostream>
#include <cstring>

#include <WinSock2.h>

#include "hbm/sys/timer.h"

namespace hbm {
	namespace sys {
		Timer::Timer()
			: m_fd(CreateWaitableTimer(NULL, FALSE, NULL))
		{
			//set(0);
			CancelWaitableTimer(m_fd);

		}

		Timer::Timer(unsigned int period_s)
			: m_fd(CreateWaitableTimer(NULL, FALSE, NULL))
		{
			set(period_s);
		}

		Timer::~Timer()
		{
			CloseHandle(m_fd);
		}

		int Timer::set(unsigned int period_s)
		{
			LARGE_INTEGER dueTime;

			dueTime.QuadPart = period_s*1000*1000*10; // in 100ns
			BOOL Result = SetWaitableTimer(
				m_fd,
				&dueTime,
				period_s*1000, // in ms
				NULL,
				NULL,
				FALSE
				);
			if(Result==0) {
				return -1;
			}
			return 0;
		}

		int Timer::wait()
		{
			//DWORD result = WaitForSingleObject(m_fd, 0);
			DWORD result = WaitForSingleObject(m_fd, INFINITE);
			switch (result) {
			case WAIT_OBJECT_0:
				return 1;
				break;
			case WAIT_ABANDONED:
				return 0;
				break;
			case WAIT_FAILED:
				std::cout << "FAILED" << std::endl;
				break;
			case WAIT_TIMEOUT:
				std::cout << "WAIT_TIMEOUT" << std::endl;
				break;
			default:
				return -1;
				break;
			}
			return 0;
		}

		int Timer::cancel()
		{
			CancelWaitableTimer(m_fd);
			return 0;
		}

		event Timer::getFd() const
		{
			return m_fd;
		}
	}
}
