// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided



#include "hbm/sys/executecommand.h"

namespace hbm {
	namespace sys {
		std::string executeCommand(const std::string& command)
		{
			std::string retVal;
#ifdef _STANDARD_HARDWARE
			std::cout << command << std::endl;
#else
			// todo: for windows
#endif
			return retVal;
		}

		int executeCommand(const std::string& command, const params_t &params, const std::string& stdinString)
		{
#ifdef _STANDARD_HARDWARE
			std::cout << command << " ";

			for(params_t::const_iterator iter = params.begin(); iter!=params.end(); ++iter) {
				std::cout << *iter << " ";
			}

			std::cout << " < " << stdinString << std::endl;
#else
			// todo: for windows
#endif
			return 0;
		}
	}
}

