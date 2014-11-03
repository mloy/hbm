#include <cstring>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h> //writev
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include <unistd.h>

#include <errno.h>
#include <syslog.h>
#include <poll.h>

#include "hbm/communication/socketnonblocking.h"


/// Maximum time to wait for connecting
const time_t TIMEOUT_CONNECT_S = 5;


hbm::communication::SocketNonblocking::SocketNonblocking()
	: m_fd(-1)
	, m_bufferedReader()
{
}

hbm::communication::SocketNonblocking::SocketNonblocking(int fd)
	: m_fd(fd)
	, m_bufferedReader()
{
}


hbm::communication::SocketNonblocking::SocketNonblocking(const std::string& fileName)
	: m_fd(-1)
	, m_bufferedReader(fileName)
{
}

hbm::communication::SocketNonblocking::~SocketNonblocking()
{
	stop();
}

int hbm::communication::SocketNonblocking::setSocketOptions()
{
	int opt = 1;

	// turn off Nagle algorithm
	setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&opt), sizeof(opt));

	opt = 12;
	// the interval between the last data packet sent (simple ACKs are not considered data) and the first keepalive probe;
	// after the connection is marked to need keepalive, this counter is not used any further
	setsockopt(m_fd, SOL_TCP, TCP_KEEPIDLE, reinterpret_cast<char*>(&opt), sizeof(opt));

	opt = 3;
	// the interval between subsequential keepalive probes, regardless of what the connection has exchanged in the meantime
	setsockopt(m_fd, SOL_TCP, TCP_KEEPINTVL, reinterpret_cast<char*>(&opt), sizeof(opt));

	opt = 2;
	// the number of unacknowledged probes to send before considering the connection dead and notifying the application layer
	setsockopt(m_fd, SOL_TCP, TCP_KEEPCNT, reinterpret_cast<char*>(&opt), sizeof(opt));

	opt = 1;
	setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<char*>(&opt), sizeof(opt));

	return 0;
}

int hbm::communication::SocketNonblocking::init()
{

	m_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if(m_fd==-1) {
		return -1;
	} else {
		setSocketOptions();
	}

	return 0;
}

int hbm::communication::SocketNonblocking::connect(const std::string &address, const std::string& port)
{
	int retVal = init();
	if(retVal<0) {
		return retVal;
	}

	struct addrinfo hints;
	struct addrinfo* pResult = NULL;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;


	if( getaddrinfo(address.c_str(), port.c_str(), &hints, &pResult)!=0 ) {
		return -1;
	}
	retVal = connect(pResult->ai_addr, sizeof(sockaddr_in));
//	int err = ::connect(m_fd, pResult->ai_addr, sizeof(sockaddr_in));
//	if(err==-1) {
//		if(errno == EINPROGRESS) {
//			// success if errno equals EINPROGRESS
//			struct pollfd pfd;
//			pfd.fd = m_fd;
//			pfd.events = POLLOUT;
//			do {
//				err = poll(&pfd, 1, TIMEOUT_CONNECT_S*1000);
//			} while((err==-1) && (errno==EINTR) );
//			if(err==1) {
//				int value;
//				socklen_t len = sizeof(value);
//				getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &value, &len);
//				if(value!=0) {
//					retVal = -1;
//				}
//			} else {
//				retVal = -1;
//			}
//		} else {
//			retVal = -1;
//		}
//	}

	freeaddrinfo( pResult );

	return retVal;
}

int hbm::communication::SocketNonblocking::connect(const struct sockaddr* pSockAddr, socklen_t len)
{
	int err = ::connect(m_fd, pSockAddr, len);
	if(err==-1) {
		if(errno == EINPROGRESS) {
			// success if errno equals EINPROGRESS
			struct pollfd pfd;
			pfd.fd = m_fd;
			pfd.events = POLLOUT;
			do {
				err = poll(&pfd, 1, TIMEOUT_CONNECT_S*1000);
			} while((err==-1) && (errno==EINTR) );
			if(err==1) {
				int value;
				socklen_t len = sizeof(value);
				getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &value, &len);
				if(value!=0) {
					err = -1;
				}
			} else {
				err = -1;
			}
		} else {
			err = -1;
		}
	}
	return err;
}

int hbm::communication::SocketNonblocking::bind(uint16_t Port)
{
	//ipv6 does work for ipv4 too!
	sockaddr_in6 address;

	memset(&address, 0, sizeof(address));
#ifdef _WIN32
	Addr.sin6_family = AF_INET;
#else
	address.sin6_family = AF_INET;
#endif
	address.sin6_addr = in6addr_any;
	address.sin6_port = htons(Port);

	int retVal = init();
	if (retVal == -1) {
		syslog(LOG_ERR, "%s: Socket initialization failed '%s'", __FUNCTION__ , strerror(errno));
		return retVal;
	}
	retVal = ::bind(m_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address));
	if (retVal == -1) {
		syslog(LOG_ERR, "%s: Binding socket to port initialization failed '%s'", __FUNCTION__ , strerror(errno));
	}
	return retVal;
}

std::unique_ptr < hbm::communication::SocketNonblocking > hbm::communication::SocketNonblocking::acceptClient()
{
	std::unique_ptr < SocketNonblocking > retSocket;

	sockaddr_in SockAddr;
	// the length of the client's address
	socklen_t socketAddressLen = sizeof(SockAddr);

	int err;

#ifdef _WIN32
	fd_set fds;

	FD_ZERO(&fds);
	FD_SET(m_socketId,&fds);

	// wir warten ohne timeout, bis etwas zu lesen ist
	do {
		err = select(static_cast < int >(m_socketId) + 1, &fds, NULL, NULL, NULL);
	} while((err==-1)&&(errno==WSAEINTR));
#else
	struct pollfd pfd;

	pfd.fd = m_fd;
	pfd.events = POLLIN;
	do {
		err = poll(&pfd, 1, -1);
	} while((err==-1) && (errno==EINTR));
#endif

	if(err!=1) {
		syslog(LOG_ERR, "%s: Poll failed!", __FUNCTION__);
		return std::unique_ptr < SocketNonblocking >();
#ifdef _WIN32
	} else if(FD_ISSET(m_socketId, &fds)) {
#else
	}	else if(pfd.revents & POLLIN) {
#endif
		// the new socket file descriptor returned by the accept system call
	#ifdef _WIN32
		SOCKET newSocketId;
	#else
		int clientFd;
	#endif

		clientFd = accept(m_fd, reinterpret_cast<sockaddr*>(&SockAddr), &socketAddressLen);
		if (clientFd >= 0) {
			std::unique_ptr < SocketNonblocking > p( new SocketNonblocking(clientFd));
			p->setSocketOptions();
			return p;
		} else {
			syslog(LOG_ERR, "%s: Accept failed!", __FUNCTION__);
		}
	}
	return std::unique_ptr < SocketNonblocking >();
}

int hbm::communication::SocketNonblocking::listenToClient(int numPorts)
{
	return listen(m_fd, numPorts);
}

ssize_t hbm::communication::SocketNonblocking::receive(void* pBlock, size_t size)
{
	return m_bufferedReader.recv(m_fd, pBlock, size, 0);
}

ssize_t hbm::communication::SocketNonblocking::receiveComplete(void* pBlock, size_t size, int msTimeout)
{
	ssize_t retVal;
	unsigned char* pPos = reinterpret_cast < unsigned char* > (pBlock);
	size_t sizeLeft = size;
	while(sizeLeft) {
		retVal = m_bufferedReader.recv(m_fd, pPos, sizeLeft, 0);
		if(retVal>0) {
			sizeLeft -= retVal;
			pPos += retVal;
		} else if(retVal==0) {
			return size-sizeLeft;
		} else {
			if(errno==EWOULDBLOCK || errno==EAGAIN) {
				// wait for socket to become readable.
				struct pollfd pfd;
				pfd.fd = m_fd;
				pfd.events = POLLIN;
				int nfds;
				do {
					nfds = poll(&pfd, 1, msTimeout);
				} while((nfds==-1) && (errno==EINTR));
				if(nfds!=1) {
					return -1;
				}
			} else {
				syslog(LOG_ERR, "%s: recv failed '%s'", __FUNCTION__, strerror(errno));
				return -1;
			}
		}
	}
	return size;
}


ssize_t hbm::communication::SocketNonblocking::sendBlock(const void* pBlock, size_t size, bool more)
{
	const uint8_t* pDat = reinterpret_cast<const uint8_t*>(pBlock);
	size_t BytesLeft = size;
	ssize_t numBytes;
	ssize_t retVal = size;

	struct pollfd pfd;
	pfd.fd = m_fd;
	pfd.events = POLLOUT;

	int flags = 0;
	if(more) {
		flags |= MSG_MORE;
	}
	int err;

	while (BytesLeft > 0) {
		numBytes = send(m_fd, pDat, BytesLeft, flags);
		if(numBytes>0) {
			pDat += numBytes;
			BytesLeft -= numBytes;
		} else if(numBytes==0) {
			// connection lost...
			BytesLeft = 0;
			retVal = -1;
		} else {
			// <0
			if(errno==EWOULDBLOCK || errno==EAGAIN) {
				// wait for socket to become writable.
				do {
					err = poll(&pfd, 1, -1);
				} while((err==-1) && (errno==EINTR));
				if(err!=1) {
					BytesLeft = 0;
					retVal = -1;
				}
			} else {
				// a real error happened!
				BytesLeft = 0;
				retVal = -1;
			}
		}
	}
	return retVal;
}


bool hbm::communication::SocketNonblocking::checkSockAddr(const struct ::sockaddr* pCheckSockAddr, socklen_t checkSockAddrLen) const
{
	struct sockaddr sockAddr;
	socklen_t addrLen = sizeof(sockaddr_in);

	char checkHost[256];
	char ckeckService[8];

	char host[256];
	char service[8];
	int err = getnameinfo(pCheckSockAddr, checkSockAddrLen, checkHost, sizeof(checkHost), ckeckService, sizeof(ckeckService), NI_NUMERICHOST | NI_NUMERICSERV);
	if (err != 0) {
		syslog(LOG_ERR, "%s: error from getnameinfo", __FUNCTION__);
		return false;
	}

	getpeername(m_fd, &sockAddr, &addrLen);

	getnameinfo(&sockAddr, addrLen, host, sizeof(host), service, sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV);

	if(
		 (strcmp(host, checkHost)==0) &&
		 (strcmp(service, ckeckService)==0)
		 )
	{
		return true;
	}
	return false;
}

void hbm::communication::SocketNonblocking::stop()
{
	::shutdown(m_fd, SHUT_RDWR);
	::close(m_fd);
	m_fd = -1;
}

bool hbm::communication::SocketNonblocking::isFirewire() const
{
	bool retVal = false;

	struct ifreq ifr;
	memset(&ifr, 0, sizeof(struct ifreq));

	if ((::ioctl(m_fd, SIOCGIFHWADDR, (caddr_t)&ifr, sizeof(struct ifreq))) >= 0) {
		if (ifr.ifr_hwaddr.sa_family == ARPHRD_IEEE1394) {
			retVal = true;
		}
	}
}
