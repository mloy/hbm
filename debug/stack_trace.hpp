/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* vim: set ts=4 et sw=4 tw=80: */
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

#include <string>

#ifdef __GNUG__
#include <execinfo.h>
#endif

namespace hbm {
	namespace debug {

		static const unsigned int backtrace_size = 100;

		static std::string fillStackTrace()
		{
			std::string output;
#ifdef __GNUG__
			void *backtrace_buffer[backtrace_size];
			unsigned int num_functions = ::backtrace(backtrace_buffer, backtrace_size);
			char **function_strings = ::backtrace_symbols(backtrace_buffer, num_functions);
			if (function_strings != NULL) {
				output.append("\n----------- stacktrace begin\n");
				output.append("\n");
				for (unsigned int i = 0; i < num_functions; ++i) {
					output.append(function_strings[i]);
					output.append("\n");
				}
				output.append("----------- stacktrace end\n");
				::free(function_strings);
			} else {
				output.append("No backtrace!\n");
			}
#else
			output.append("No backtrace!\n");
#endif
			return output;
		}
	}
}
