/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
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



#ifndef HBM__ERRNO_EXCEPTION_H
#define HBM__ERRNO_EXCEPTION_H


#include <cstring>
#include <string>

#include <errno.h>

#include "exception.hpp"

namespace hbm {
	namespace exception {

	/// exception which reads back errno stores its value and retrieves the
	/// corresponding error string which will be returned by
	/// virtual base method std::exception::what.
	/// suitable if dealing with naked unix methods which returns an error
	/// and as base-class for more specific exceptions.
	/// usage:
	/// if( write( fd, "abc", sizeof("abc") ) == -1 )
		///    throw errno_exception();
		class errno_exception : public hbm::exception::exception {

		public:
			errno_exception()
				: hbm::exception::exception(::strerror(errno)),
				  _errorno(errno)
			{
			}

			virtual ~errno_exception() throw() {}

			int errorno() const {
				return _errorno;
			}
		private:
			const int _errorno;
		};
	}
}

#endif

