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


#ifndef HBM__EXCEPTION_H
#define HBM__EXCEPTION_H

#include <stdexcept>
#include <string>

#ifdef __GNUG__
#include <cstdlib>

#include <execinfo.h>
#endif

#include "hbm/debug/stack_trace.hpp"

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
				exception(const std::string& description)
					: std::runtime_error(description)
					, output(std::string(std::runtime_error::what()))
				{
					output.append("\n");
					output.append(hbm::debug::fillStackTrace());
				}
				virtual ~exception() throw() {}
				virtual const char* what() const throw()
				{
					return output.c_str();
				}
			protected:
				std::string output;

			private:
				static const unsigned int backtrace_size = 100;
		};
	}
}

#endif

