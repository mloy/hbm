// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#include <iostream>
#include <cstring>

#include <WinSock2.h>

#include "hbm/sys/notifier.h"

namespace hbm {
	namespace sys {
		Notifier::Notifier()
			: m_fd(NULL)
		{
			m_fd = CreateEvent(NULL, false, false, NULL);
		}

		Notifier::~Notifier()
		{
			CloseHandle(m_fd);
		}

		int Notifier::notify()
		{
			if (SetEvent(m_fd)==0) {
				return -1;
			}
			return 0;
		}

		int Notifier::wait()
		{
			DWORD result = WaitForSingleObject(m_fd, INFINITE);
			if (result != WAIT_OBJECT_0) {
				return -1;
			}
			return 0;
		}		

		event Notifier::getFd() const
		{
			return m_fd;
		}
	}
}
