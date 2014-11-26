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

/** @file */


#ifndef _WIN32
#define BOOST_TEST_DYN_LINK
#endif
#define BOOST_TEST_MAIN

#define BOOST_TEST_MODULE eventloop tests

#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "hbm/sys/eventloop.h"
#include "hbm/sys/timer.h"
#include "hbm/exception/exception.hpp"




/// by returning error, the execute() method, that is doing the eventloop, exits
static ssize_t eventHandlerStop()
{
	return -1;
}



BOOST_AUTO_TEST_CASE(waitforend_test)
{
	hbm::sys::EventLoop eventLoop;

	static const boost::posix_time::milliseconds duration(3000);


	boost::posix_time::ptime startTime = boost::posix_time::microsec_clock::universal_time();
	int result = eventLoop.execute(duration);
	boost::posix_time::ptime endTime = boost::posix_time::microsec_clock::universal_time();

	unsigned int delta = (endTime - startTime - duration).total_milliseconds();
	BOOST_CHECK(result==0);
	BOOST_CHECK(delta<5);
}

BOOST_AUTO_TEST_CASE(timerevent_test)
{
	static const unsigned int timerCycle = 1;
	static const unsigned int timerCount = 10;
	static const boost::posix_time::milliseconds duration(timerCycle*3000);
	hbm::sys::EventLoop eventLoop;

	hbm::sys::Timer timer(timerCycle);
	eventLoop.addEvent(timer.getFd(), &eventHandlerStop);

	int result = eventLoop.execute(duration);
	BOOST_CHECK(result==-1);
}
