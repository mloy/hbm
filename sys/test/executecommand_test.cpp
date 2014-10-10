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

#define BOOST_TEST_MODULE executecommand tests

#include <cstdio>
#include <iostream>
#include <boost/test/unit_test.hpp>

#include "hbm/sys/executecommand.h"
#include "hbm/exception/exception.hpp"


BOOST_AUTO_TEST_CASE(command_withparameters_test)
{
	static const std::string fileName = "bla";
	::remove(fileName.c_str());
	hbm::sys::executeCommand("/usr/bin/touch " + fileName);
	int result = ::remove(fileName.c_str());
	BOOST_CHECK(result==0);
}


BOOST_AUTO_TEST_CASE(invalid_command_test)
{
	static const std::string fileName = "bla";
	hbm::sys::params_t params;
	params.push_back(fileName);
	int result = hbm::sys::executeCommand("/usr/bin/touc", params, "");
	BOOST_CHECK(result==-1);
}

BOOST_AUTO_TEST_CASE(valid_command_test)
{
	static const std::string fileName = "bla";
	int result = ::remove(fileName.c_str());
	hbm::sys::params_t params;
	params.push_back(fileName);

	result = hbm::sys::executeCommand("/usr/bin/touch", params, "");
	BOOST_CHECK(result==0);
	result = ::remove(fileName.c_str());
	BOOST_CHECK(result==0);
}

BOOST_AUTO_TEST_CASE(stdin_test)
{
	hbm::sys::params_t params;

	params.push_back("-w");
	int result = hbm::sys::executeCommand("/usr/bin/wc", params, "bla blub");
	BOOST_CHECK(result==0);
}

BOOST_AUTO_TEST_CASE(answer_test)
{
	std::string cmd = "/bin/echo";
	std::string arg = "hallo";
	std::string result = hbm::sys::executeCommand(cmd + " " + arg);
	BOOST_CHECK(result.substr(0, result.size()-1)==arg);
}
