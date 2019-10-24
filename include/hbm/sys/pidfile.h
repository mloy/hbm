// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


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
			PidFile(const char* name);
			/// \return absolute path of the pid file
			std::string name();
			virtual ~PidFile();

		private:
			PidFile(PidFile& el);
			PidFile operator=(PidFile& el);

			std::string m_pidFileName;
		};
	}
}
#endif
