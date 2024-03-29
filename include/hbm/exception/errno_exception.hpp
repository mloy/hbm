// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided



#ifndef HBM__ERRNO_EXCEPTION_H
#define HBM__ERRNO_EXCEPTION_H



#include "hbm/exception/exception.hpp"

namespace hbm {
	namespace exception {
		/// exception which reads back errno stores its value and retrieves the
		/// corresponding error string which will be returned by
		/// virtual base method std::exception::what.
		/// suitable if dealing with naked unix methods which returns an error
		/// and as base-class for more specific exceptions.
		/// usage:
		/// if( write( fd, "abc", sizeof("abc") ) == -1 ) {
		///    throw errno_exception();
		/// }
		class errno_exception : public hbm::exception::exception {

		public:
			errno_exception();

			virtual ~errno_exception() noexcept;

			int errorno() const noexcept;
		private:
			const int _errorno;
		};
	}
}

#endif

