// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#ifndef __HBM__SOCKETNONBLOCKING_H
#define __HBM__SOCKETNONBLOCKING_H

#include <list>
#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#undef max
#undef min
#ifndef ssize_t
typedef int ssize_t;
#endif
#else
#include <sys/socket.h>
#endif

#include "hbm/communication/bufferedreader.h"
#include "hbm/sys/eventloop.h"

namespace hbm
{
	namespace communication {
		/// used for scatter gather operations
		struct dataBlock_t {
			dataBlock_t()
				: pData(NULL)
				, size(0)
			{
			}

			/// All members are initialized on construction
			dataBlock_t(const void* pD, size_t s)
				: pData(pD)
				, size(s)
			{
			}

			/// Data buffer
			const void* pData;
			/// Size of the data buffer
			size_t size;
		};

		typedef std::list < dataBlock_t > dataBlocks_t;

		/// A tcp client connection to a tcpserver. Ipv4 and ipv6 are supported.
		/// the socket uses keep-alive in order to detect broken connection.
		class SocketNonblocking
		{
		public:
			/// called on the arrival of data
			typedef std::function < ssize_t (SocketNonblocking& socket) > DataCb_t;
			/// @param eventLoop Event loop the object will be registered in 
			SocketNonblocking(sys::EventLoop &eventLoop);

			/// used when accepting connection via tcp server.
			/// \throw std::runtime_error on error
			SocketNonblocking(int fd, sys::EventLoop &eventLoop);
			virtual ~SocketNonblocking();

			/// this method does work blocking
			/// \return 0: success; -1: error
			int connect(const std::string& address, const std::string& port);

			/// this method does work blocking
			int connect(int domain, const struct sockaddr* pSockAddr, socklen_t len);

			/// Remove event from event loop and close socket
			void disconnect();

			/// if setting a callback function, data receiption is done via event loop.
			/// if setting an empty callback function DataCb_t(), the event is taken out of the eventloop.
			/// \param dataCb callback to be called if fd gets readable (data is available)
			void setDataCb(DataCb_t dataCb);
			
			/// \param dataCb callback to be called if fd gets writable
			void setOutDataCb(DataCb_t dataCb);

			/// send everything or until connection closes
			/// uses gather mechanism to send several memory areas
			ssize_t sendBlocks(const dataBlocks_t& blocks);

			/// send everything or until connection closes
			/// uses gather mechanism to send several memory areas
			ssize_t sendBlocks(const dataBlock_t *blocks, size_t blockCount);

			/// send everything or until connection closes
			/// \warning waits until requested amount of data is processed or an error happened, hence it might block the eventloop if called from within a callback function
			ssize_t sendBlock(const void* pBlock, size_t len, bool more);

			/// works as posix send
			ssize_t send(const void* pBlock, size_t len, bool more);

			/// might return with less bytes the requested
			ssize_t receive(void* pBlock, size_t len);

			/// might return with less bytes then requested if connection is being closed before completion
			/// \warning waits until requested amount of data is processed or an error happened, hence it might block the eventloop if called from within a callback function
			/// @param pBlock Receive buffer
			/// @param len Lenght of receive buffer
			/// @param msTimeout -1 for infinite
			ssize_t receiveComplete(void* pBlock, size_t len, int msTimeout = -1);

			/// \return true if socket uses firewire connection
			bool isFirewire() const;

			/// @param pCheckSockAddr The structure to compare the socket of this object with
			/// @param checkSockAddrLen Length of the structure depends on the type of socket (ipv4, ipv6)
			/// \return true if the socket of this object corresponds to the given sockaddr structure
			bool checkSockAddr(const struct sockaddr* pCheckSockAddr, socklen_t checkSockAddrLen) const;

			/// \return the file descriptor (Linux) or handle (Microsoft Windows)
			sys::event getEvent() const
			{
				return m_event;
			}
			
private:			
			/// should not be copied
			SocketNonblocking(const SocketNonblocking& op);

			/// should not be assigned
			SocketNonblocking& operator= (const SocketNonblocking& op);

			int setSocketOptions();

			sys::event m_event;

			BufferedReader m_bufferedReader;

			sys::EventLoop& m_eventLoop;
			DataCb_t m_inDataHandler;
			DataCb_t m_outDataHandler;
		};
		
#ifdef _MSC_VER
		typedef std::shared_ptr <SocketNonblocking > clientSocket_t;
#else
		typedef std::unique_ptr <SocketNonblocking > clientSocket_t;
#endif
		
	}
}
#endif
