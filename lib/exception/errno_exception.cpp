// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <cstring>
#include <string>

#include <errno.h>

#include "hbm/exception/errno_exception.hpp"
#include "hbm/exception/exception.hpp"

namespace hbm {
	namespace exception {
		errno_exception::errno_exception()
			: hbm::exception::exception(::strerror(errno))
			, _errorno(errno)
		{
		}

		errno_exception::~errno_exception() noexcept
		{
		}

		int errno_exception::errorno() const noexcept
		{
			return _errorno;
		}
	}
}
