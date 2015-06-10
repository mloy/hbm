// Copyright 2015 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#include "split.h"


namespace hbm {
	namespace string {

		std::vector<std::string> split(std::string text, char separator)
		{
			std::vector<std::string> tokens;

			size_t pos_start=0;

			while(1)
			{
				size_t pos_end = text.find(separator, pos_start);
				std::string token = text.substr(pos_start, pos_end-pos_start);
				tokens.push_back(token);
				if(pos_end == std::string::npos) break;
				pos_start = pos_end+1;
			}
			return tokens;
		}

		tokens split(std::string text, const std::string& separator)
		{
			tokens result;

			if(separator.length() == 0)
			{
				result.push_back(text);
				return result;
			}

			size_t pos_start=0;

			while(1)
			{
				size_t pos_end = text.find(separator, pos_start);
				std::string token = text.substr(pos_start, pos_end-pos_start);
				result.push_back(token);
				if(pos_end == std::string::npos) break;
				pos_start = pos_end+separator.length();
			}
			return result;
		}
	}
}

