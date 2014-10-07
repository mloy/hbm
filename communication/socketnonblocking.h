#ifndef __HBM__SOCKETNONBLOCKING_H
#define __HBM__SOCKETNONBLOCKING_H

#include <string>


#ifdef _WIN32
#include <WinSock2.h>
#undef max
#undef min
#endif



#include "hbm/communication/bufferedreader.h"
#include "hbm/sys/eventloop.h"

namespace hbm
{
	namespace communication {
		/// the socke uses keep-alive in order to detect broken connection.
		class SocketNonblocking
		{
		public:
			SocketNonblocking();

			/// use this variant to dump everything to a file
			/// \throw std::runtime_error if file could not be opened
			SocketNonblocking(const std::string& fileName);
			virtual ~SocketNonblocking();

			/// \return 0: success; -1: error
			int connect(const std::string& address, const std::string& port);

			ssize_t sendBlock(const void* pBlock, size_t len, bool more);

			/// might return with less bytes the requested
			ssize_t receive(void* pBlock, size_t len);

			/// might return with less bytes the requested if connection is being closed before completion
			ssize_t receiveComplete(void* pBlock, size_t len);

	#ifdef _WIN32
			event getFd() const
			{
				return m_event;
			}
	#else
			event getFd() const
			{
				return m_fd;
			}
	#endif

			void stop();

		protected:
			/// should not be copied
			SocketNonblocking(const SocketNonblocking& op);

			/// should not be assigned
			SocketNonblocking& operator= (const SocketNonblocking& op);

			int init();

			int m_fd;
			#ifdef _WIN32
					WSAEVENT m_event;
			#endif

			BufferedReader m_bufferedReader;
		};
	}
}
#endif
