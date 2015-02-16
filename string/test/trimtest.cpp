// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <iostream>
#include <vector>


#ifndef _WIN32
#define BOOST_TEST_DYN_LINK
#endif
#define BOOST_TEST_MODULE StringTrimTest
#include <boost/test/unit_test.hpp>


#include <iostream>

#include "../trim.h"


namespace hbm {
	namespace string {
		namespace test {


			struct Fixture1
			{
			public:
				Fixture1()
				{
					BOOST_TEST_MESSAGE("setup Fixture1");
				}

				~Fixture1()
				{
					BOOST_TEST_MESSAGE("teardown Fixture1");
				}
			};

			BOOST_FIXTURE_TEST_SUITE( Fixture1_Test, Fixture1 )

			BOOST_AUTO_TEST_CASE( test_case_right )
			{
				std::string result = hbm::string::trim("hallo  ");
				BOOST_CHECK_EQUAL(result, "hallo");
			}

			BOOST_AUTO_TEST_CASE( test_case_left )
			{
				std::string result = hbm::string::trim("  hallo");
				BOOST_CHECK_EQUAL(result, "hallo");
			}

			BOOST_AUTO_TEST_CASE( test_case_both )
			{
				std::string result = hbm::string::trim("  hallo  ");
				BOOST_CHECK_EQUAL(result, "hallo");
			}

			BOOST_AUTO_TEST_CASE( test_case_nothing )
			{
				std::string result = hbm::string::trim("hallo");
				BOOST_CHECK_EQUAL(result, "hallo");
			}


			BOOST_AUTO_TEST_CASE( test_case_empty )
			{
				std::string result = hbm::string::trim("    ");
				BOOST_CHECK_EQUAL(result, "");
			}


			BOOST_AUTO_TEST_SUITE_END()
		}
	}
}
