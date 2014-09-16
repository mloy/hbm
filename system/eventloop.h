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

#ifndef _EventLoop_H
#define _EventLoop_H

#include <unordered_map>

#ifdef _WIN32
#include <Windows.h>
typedef HANDLE event;
#else
typedef int event;
#endif

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/function.hpp>

#include "hbm/exception/exception.hpp"

namespace hbm {
	typedef boost::function < ssize_t () > eventHandler_t;


	/// \warning not thread-safe
	class EventLoop {
	public:
		/// \throws hbm::exception
		EventLoop();
		virtual ~EventLoop();
		void addEvent(event fd, eventHandler_t eventHandler);

		void eraseEvent(event fd);

		void clear();

		int execute(boost::posix_time::milliseconds timeToWait=boost::posix_time::milliseconds(0));
	private:
		struct eventInfo_t {
			event fd;
			eventHandler_t eventHandler;
		};

		/// fd is the key
		typedef std::unordered_map <int, eventInfo_t > eventInfos_t;

		int m_epollfd;
		eventInfos_t m_eventInfos;
	};
}
#endif
