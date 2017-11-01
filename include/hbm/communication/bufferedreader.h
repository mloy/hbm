// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#ifndef _HBM__BUFFEREDREADER_H
#define _HBM__BUFFEREDREADER_H

#include "hbm/sys/defines.h"
#ifdef _WIN32
#ifndef ssize_t
#define ssize_t int
#endif
#else
#include <sys/types.h>
#endif

namespace hbm {
	namespace communication {
		/// Try to receive a big chunk even if only a small amount of data is requested.
		/// This reduces the number of system calls being made.
		/// Return the requested amount of data and keep the remaining data.
		/// \warning not reentrant
		class BufferedReader
		{
		public:
			BufferedReader();

			/// behaves like standard recv
			ssize_t recv(hbm::sys::event& ev, void *buf, size_t len);

		private:
			BufferedReader(const BufferedReader& op);
			BufferedReader& operator=(const BufferedReader& op);

			unsigned char m_buffer[65536];
			size_t m_fillLevel;
			size_t m_alreadyRead;
		};
	}
}
#endif // BUFFEREDREADER_H
