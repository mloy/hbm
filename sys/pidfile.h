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


#ifndef _PIDFILE_H
#define _PIDFILE_H

#include <string>

namespace hbm {
	namespace sys {
		/// creates pid file on construction deletes pid file on destruction
		class PidFile
		{
		public:
			/// \throws hbm::exception
			PidFile(const std::string& programName);
			virtual ~PidFile();

		private:
			std::string m_pidFileName;
		};
	}
}
#endif
