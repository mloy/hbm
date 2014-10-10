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


#ifndef _EXECUTECOMMAND_H
#define _EXECUTECOMMAND_H

#include <string>
#include <vector>

namespace hbm {
	namespace sys {
		typedef std::vector < std::string > params_t;
		/// \throws hbm::exception
		void executeCommand(const std::string& command);

		int executeCommand(const std::string& command, const params_t& params, const std::string& stdinString);

		/// \throws hbm::exception
		std::string executeCommandWithAnswer(const std::string& command);
	}
}
#endif
