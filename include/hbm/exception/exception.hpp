// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided



#ifndef HBM__EXCEPTION_H
#define HBM__EXCEPTION_H

#include <stdexcept>
#include <string>

namespace hbm {
	namespace exception {
		/// base class for all exceptions raised/thrown by hbm-code.
		/// this base class is required for catching all hbm-specific exceptions with
		/// one catch clause like
		/// try
		/// {
		/// }
		/// catch( const hbm::exception& e )
		/// {
		/// }
		class exception : public std::runtime_error {
			public:
				exception(const std::string& description);
				virtual ~exception() noexcept;
				virtual const char* what() const noexcept;
			protected:
				std::string output;

			private:
				static const unsigned int backtrace_size = 100;
		};
	}
}

#endif

