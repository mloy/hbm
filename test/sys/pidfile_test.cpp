// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#include <sys/stat.h>


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
				std::string path;
				{
					hbm::sys::PidFile pidFile("fritz");
					path = pidFile.path();
					int result = access(path.c_str(), F_OK);
					BOOST_CHECK(result==0);
				}
				int result = access(path.c_str(), F_OK);
				BOOST_CHECK(result!=0);
			}
		}

		BOOST_AUTO_TEST_CASE( check_invalid_path )
		{
			char baseName[] = "fritz";
			hbm::sys::PidFile pidFile(baseName);
			std::string path = pidFile.path();
			chmod(path.c_str(), 0); // protect file from being overwritten!
			BOOST_CHECK_THROW(hbm::sys::PidFile pidFile2(baseName), std::runtime_error);
		}

	}
}
