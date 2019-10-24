// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#include <cstdio>
#include <cstring>

#include <stdexcept>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#define basename(x) x
#else
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#endif


#include "hbm/sys/pidfile.h"

#ifdef _STANDARD_HARDWARE
	// use current directory on pc because we hopefully have write access here.
	const char BasePath[] = "";
#else
	const char BasePath[] = "/var/run/";
#endif

namespace hbm {
	namespace sys {
		PidFile::PidFile(const char* name)
			: m_pidFileName(BasePath)
		{
			char* pNameCopy = strdup(name);
			m_pidFileName += basename(pNameCopy);
			free(pNameCopy);
			m_pidFileName += ".pid";
			FILE* pidFile = ::fopen(m_pidFileName.c_str(), "w");

			if (pidFile == nullptr) {
				std::string msg;
				msg = "could not create pid file ";
				msg += m_pidFileName;
				msg += std::string(" '") +strerror(errno) + "'";
				throw std::runtime_error(msg);
			} else {
				::fprintf(pidFile, "%d\n", getpid());
				::fclose(pidFile);
			}
		}

		std::string PidFile::path()
		{
			return m_pidFileName;
		}

		PidFile::~PidFile()
		{
			::remove(m_pidFileName.c_str());
		}
	}
}
