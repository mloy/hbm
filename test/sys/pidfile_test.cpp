// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided



#ifndef _WIN32
#define BOOST_TEST_DYN_LINK
#endif
#define BOOST_TEST_MODULE StringReplaceTest
#include <boost/test/unit_test.hpp>



#include "hbm/sys/pidfile.h"


namespace hbm {
	namespace sys {
		namespace test {
			BOOST_AUTO_TEST_CASE( check_creation_and_removal )
			{
				std::string name;
				{
					hbm::sys::PidFile pidFile("fritz");
					name = pidFile.name();
					int result = access(name.c_str(), F_OK);
					BOOST_CHECK(result==0);
				}
				int result = access(name.c_str(), F_OK);
				BOOST_CHECK(result!=0);
			}
		}
	}
}
