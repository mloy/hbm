// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <memory>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <errno.h>
#include <syslog.h>

#include "hbm/communication/socketnonblocking.h"
#include "hbm/communication/tcpserver.h"
#include "hbm/sys/eventloop.h"


namespace hbm {
	namespace communication {
		TcpServer::TcpServer(sys::EventLoop &eventLoop)
			: m_listeningEvent(-1)
			, m_eventLoop(eventLoop)
			, m_acceptCb()
		{
		}

		TcpServer::~TcpServer()
		{
			stop();
		}

		int TcpServer::start(uint16_t port, int backlog, Cb_t acceptCb)
		{
			if (!acceptCb) {
				return -1;
			}

			//ipv6 does work for ipv4 too!
			sockaddr_in6 address;
			memset(&address, 0, sizeof(address));
			address.sin6_family = AF_INET6;
			address.sin6_addr = in6addr_any;
			address.sin6_port = htons(port);

			m_listeningEvent = ::socket(address.sin6_family, SOCK_STREAM | SOCK_NONBLOCK, 0);
			if (m_listeningEvent==-1) {
				::syslog(LOG_ERR, "server: Socket initialization failed '%s'", strerror(errno));
				return -1;
			}
			
			uint32_t yes = 1;
			// important for start after stop. Otherwise we have to wait some time until the port is really freed by the operating system.
			if (setsockopt(m_listeningEvent, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
				::syslog(LOG_ERR, "server: Could not set SO_REUSEADDR!");
				return -1;
			}
			
			if (::bind(m_listeningEvent, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
				::syslog(LOG_ERR, "server: Binding socket to port %u failed '%s'", port, strerror(errno));
				return -1;
			}
			if (listen(m_listeningEvent, backlog)==-1) {
				return -1;
			}
			m_acceptCb = acceptCb;
			m_eventLoop.addEvent(m_listeningEvent, std::bind(&TcpServer::process, this));
			return 0;
		}

		int TcpServer::start(const std::string& path, int backlog, Cb_t acceptCb)
		{
			sockaddr_un address;
			memset(&address, 0, sizeof(address));
			address.sun_family = AF_UNIX;
			strncpy(address.sun_path, path.c_str(), sizeof(address.sun_path)-1);
			m_listeningEvent = ::socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
			if (m_listeningEvent==-1) {
				::syslog(LOG_ERR, "server: Socket initialization failed '%s'", strerror(errno));
				return -1;
			}

			uint32_t yes = 1;
			// important for start after stop. Otherwise we have to wait some time until the port is really freed by the operating system.
			if (setsockopt(m_listeningEvent, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
				::syslog(LOG_ERR, "server: Could not set SO_REUSEADDR!");
				return -1;
			}

			::unlink(path.c_str());
			if (::bind(m_listeningEvent, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
				::syslog(LOG_ERR, "server: Binding socket to unix domain socket %s failed '%s'", path.c_str(), strerror(errno));
				return -1;
			}
			chmod(path.c_str(), 0666); // everyone should have access
			if (listen(m_listeningEvent, backlog)==-1) {
				return -1;
			}
			m_path = path;
			m_acceptCb = acceptCb;
			m_eventLoop.addEvent(m_listeningEvent, std::bind(&TcpServer::process, this));
			return 0;
		}

		void TcpServer::stop()
		{
			m_eventLoop.eraseEvent(m_listeningEvent);
			::close(m_listeningEvent);
			if (m_path.empty()==false) {
				// unlink unix domain socket
				::unlink(m_path.c_str());
			}
			m_acceptCb = Cb_t();
		}

		int TcpServer::process()
		{
			int clientFd = ::accept(m_listeningEvent, nullptr, nullptr);
			if (clientFd==-1) {
				if ((errno!=EWOULDBLOCK) && (errno!=EAGAIN) && (errno!=EINTR) ) {
					::syslog(LOG_ERR, "server: error accepting connection '%s'", strerror(errno));
				}
				return -1;
			}
			m_acceptCb(clientSocket_t(new SocketNonblocking(clientFd, m_eventLoop)));
			// we are working edge triggered. Returning > 0 tells the eventloop to call process again to try whether there is more in the queue.
			return 1;
		}
	}
}
