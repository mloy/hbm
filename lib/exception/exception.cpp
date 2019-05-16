// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include "hbm/exception/exception.hpp"
#include "hbm/debug/stack_trace.hpp"

namespace hbm {
	namespace exception {
		exception::exception(const std::string& description)
			: std::runtime_error(description)
			, output(std::string(std::runtime_error::what()))
		{
			output.append("\n");
			output.append(hbm::debug::fill_stack_trace());
		}

		exception::~exception()
		{
		}

		const char* exception::what() const noexcept
		{
			return output.c_str();
		}
	}
}
