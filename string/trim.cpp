// Copyright 2015 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#include <string>

#include "trim.h"


namespace hbm {
	namespace string {
		std::string trim(std::string text)
		{
			std::string::size_type start = text.find_first_not_of(' ');
			if (start==std::string::npos) {
				return "";
			}
			std::string::size_type end = text.find_last_not_of(' ');
			std::string::size_type length = end-start+1;

			return text.substr(start, length);
		}
	}
}

