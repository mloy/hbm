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

#include <cstdio>

#ifdef _WIN32
#define LOG_DEBUG stdout
#define LOG_INFO stdout
#define LOG_ERR stderr
#include <process.h>
#define getpid _getpid
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#include "hbm/exception/exception.hpp"

#include "hbm/sys/pidfile.h"

namespace hbm {
	namespace sys {
		PidFile::PidFile(const std::string& baseName)
	#ifdef _HBM_HARDWARE
			: m_pidFileName("/var/run/")
	#else
			: m_pidFileName()
	#endif
		{
			m_pidFileName += baseName + ".pid";
			FILE* pidFile = ::fopen(m_pidFileName.c_str(), "w");

			if (pidFile == NULL) {
				std::string msg;
				msg = "could not create pid file ";
				msg += m_pidFileName;
				throw hbm::exception::exception(msg);
			} else {
				::fprintf(pidFile, "%d\n", getpid());
				::fclose(pidFile);
			}
		}

		PidFile::~PidFile()
		{
			::remove(m_pidFileName.c_str());
		}
	}
}
