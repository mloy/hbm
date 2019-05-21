// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <cstring>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include <unistd.h>
#include <fcntl.h>
#include <vector>

#include <errno.h>
#include <syslog.h>
#include <poll.h>

#include "hbm/communication/socketnonblocking.h"


//#define WRITEV_TEST

/// Maximum time to wait for connecting
const time_t TIMEOUT_CONNECT_S = 5;


hbm::communication::SocketNonblocking::SocketNonblocking(sys::EventLoop &eventLoop)
	: m_event(-1)
	, m_bufferedReader()
	, m_eventLoop(eventLoop)
{
}

hbm::communication::SocketNonblocking::SocketNonblocking(int fd, sys::EventLoop &eventLoop)
	: m_event(fd)
	, m_bufferedReader()
	, m_eventLoop(eventLoop)
{
	if (m_event==-1) {
		throw std::runtime_error("not a valid socket");
	}
	if (fcntl(m_event, F_SETFL, O_NONBLOCK)==-1) {
		throw std::runtime_error("error setting socket to non-blocking");
	}
	if (setSocketOptions()<0) {
		throw std::runtime_error("error setting socket options");
	}
}

hbm::communication::SocketNonblocking::~SocketNonblocking()
{
	disconnect();
}

void hbm::communication::SocketNonblocking::setDataCb(DataCb_t dataCb)
{
	m_inDataHandler = dataCb;
	m_eventLoop.addEvent(m_event, std::bind(dataCb, std::ref(*this)));
}

void hbm::communication::SocketNonblocking::clearDataCb()
{
	m_inDataHandler = DataCb_t();
	m_eventLoop.eraseEvent(m_event);
}

void hbm::communication::SocketNonblocking::setOutDataCb(DataCb_t dataCb)
{
	m_outDataHandler = dataCb;
	m_eventLoop.addOutEvent(m_event, std::bind(dataCb, std::ref(*this)));
}

void hbm::communication::SocketNonblocking::clearOutDataCb()
{
	m_inDataHandler = DataCb_t();
	m_eventLoop.eraseOutEvent(m_event);
}

int hbm::communication::SocketNonblocking::setSocketOptions()
{
	int opt = 1;

	struct sockaddr_storage sockAddr;
	socklen_t sockAddrSize = sizeof(sockAddr);
	if (getsockname(m_event, reinterpret_cast< struct sockaddr * > (&sockAddr), &sockAddrSize) < 0) {
		syslog(LOG_ERR, "could not determine socket domain %s", strerror(errno));
		return -1;
	}
	if ((sockAddr.ss_family == AF_INET) || (sockAddr.ss_family == AF_INET6)) {
		// those are relevant for ip sockets only:

		// turn off Nagle algorithm
		if (setsockopt(m_event, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&opt), sizeof(opt))==-1) {
			syslog(LOG_ERR, "error turning off nagle algorithm %s", strerror(errno));
			return -1;
		}

		opt = 12;
		// the interval between the last data packet sent (simple ACKs are not considered data) and the first keepalive probe;
		// after the connection is marked to need keepalive, this counter is not used any further
		if (setsockopt(m_event, SOL_TCP, TCP_KEEPIDLE, reinterpret_cast<char*>(&opt), sizeof(opt))==-1) {
			syslog(LOG_ERR, "error setting socket option TCP_KEEPIDLE");
			return -1;
	}


		opt = 3;
		// the interval between subsequential keepalive probes, regardless of what the connection has exchanged in the meantime
		if (setsockopt(m_event, SOL_TCP, TCP_KEEPINTVL, reinterpret_cast<char*>(&opt), sizeof(opt))==-1) {
			syslog(LOG_ERR, "error setting socket option TCP_KEEPINTVL");
			return -1;
		}


		opt = 2;
		// the number of unacknowledged probes to send before considering the connection dead and notifying the application layer
		if (setsockopt(m_event, SOL_TCP, TCP_KEEPCNT, reinterpret_cast<char*>(&opt), sizeof(opt))==-1) {
			syslog(LOG_ERR, "error setting socket option TCP_KEEPCNT");
			return -1;
		}
	}


	opt = 1;
	if (setsockopt(m_event, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<char*>(&opt), sizeof(opt))==-1) {
		syslog(LOG_ERR, "error setting socket option SO_KEEPALIVE");
		return -1;
	}

	return 0;
}


int hbm::communication::SocketNonblocking::connect(const std::string &address, const std::string& port)
{
	// tcp
	struct addrinfo hints;
	struct addrinfo* pResult = nullptr;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 6; // Ip V6!

	if( getaddrinfo(address.c_str(), port.c_str(), &hints, &pResult)!=0 ) {
		return -1;
	}
	int retVal = connect(pResult->ai_family, pResult->ai_addr, pResult->ai_addrlen);
	if (retVal < 0) {
		syslog(LOG_ERR, "could not connect to tcp socket: '%s'", strerror(errno));
	}
	freeaddrinfo( pResult );
	return retVal;
}

int hbm::communication::SocketNonblocking::connect(const std::string &path)
{
	// unix domain socket!
	struct sockaddr_un sockaddr;

	memset(&sockaddr, 0, sizeof(sockaddr));

	sockaddr.sun_family = AF_UNIX;
	strncpy(sockaddr.sun_path, path.c_str(), sizeof(sockaddr.sun_path)-1);
	int retVal = connect(AF_UNIX, reinterpret_cast < struct sockaddr* > (&sockaddr), sizeof(sockaddr));
	if (retVal < 0) {
		syslog(LOG_ERR, "could not connect to unix domain socket '%s': '%s'", path.c_str(), strerror(errno));
	}
	return retVal;
}

int hbm::communication::SocketNonblocking::connect(int domain, const struct sockaddr* pSockAddr, socklen_t len)
{
	m_event = ::socket(domain, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (m_event==-1) {
		return -1;
	}

	// callback functions might be set before fd was created.
	if (m_inDataHandler) {
		m_eventLoop.addEvent(m_event, std::bind(m_inDataHandler, std::ref(*this)));
	}

	if (m_outDataHandler) {
		m_eventLoop.addOutEvent(m_event, std::bind(m_outDataHandler, std::ref(*this)));
	}

	if (setSocketOptions()<0) {
		return -1;
	}

	int err = ::connect(m_event, pSockAddr, len);
	if (err==-1) {
		// success if errno equals EINPROGRESS
		if(errno != EINPROGRESS) {
			syslog(LOG_ERR, "failed to connect socket (errno=%d '%s')", errno, strerror(errno));
			return -1;
		}
		struct pollfd pfd;
		pfd.fd = m_event;
		pfd.events = POLLOUT;
		do {
			err = poll(&pfd, 1, TIMEOUT_CONNECT_S*1000);
		} while((err==-1) && (errno==EINTR) );
		if(err!=1) {
			return -1;
		}

		int value;
		len = sizeof(value);
		getsockopt(m_event, SOL_SOCKET, SO_ERROR, &value, &len);
		if(value!=0) {
			return -1;
		}
	}
	return 0;
}

ssize_t hbm::communication::SocketNonblocking::receive(void* pBlock, size_t size)
{
	return m_bufferedReader.recv(m_event, pBlock, size);
}

ssize_t hbm::communication::SocketNonblocking::receiveComplete(void* pBlock, size_t size, int msTimeout)
{
	ssize_t retVal;
	unsigned char* pPos = reinterpret_cast < unsigned char* > (pBlock);
	size_t sizeLeft = size;
	while (sizeLeft) {
		retVal = m_bufferedReader.recv(m_event, pPos, sizeLeft);
		if (retVal>0) {
			sizeLeft -= static_cast < size_t > (retVal);
			pPos += retVal;
		} else if (retVal==0) {
			return static_cast < int > (size-sizeLeft);
		} else {
			if(errno==EWOULDBLOCK || errno==EAGAIN) {
				// wait for socket to become readable.
				struct pollfd pfd;
				pfd.fd = m_event;
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
	return static_cast < ssize_t > (size);
}


ssize_t hbm::communication::SocketNonblocking::sendBlocks(const dataBlock_t *blocks, size_t blockCount, bool more)
{
	size_t completeLength = 0;

	for(size_t blockIndex = 0; blockIndex<blockCount; ++blockIndex) {
		completeLength += blocks[blockIndex].size;
	}

	ssize_t retVal;
#ifdef WRITEV_TEST
	// for testing reasons, we want to send a part only. The rest is to be processed afterwards!
	ssize_t internelRetVal;
	size_t blockIndex;
	retVal = 0;
	for (blockIndex = 0; blockIndex<blockCount/2; ++blockIndex) {
		internelRetVal = sendBlock (blocks[blockIndex].pData, blocks[blockIndex].size, more);
		if (internelRetVal<=0) {
			return 0;
		} else {
			retVal += internelRetVal;
		}
	}
	internelRetVal = sendBlock (blocks[blockIndex].pData, blocks[blockIndex].size/2, more);
	if (internelRetVal<=0) {
		return 0;
	} else {
		retVal += internelRetVal;
	}
#else
	if (more) {
		// we use sendmsg instead of writev because we want to set the flag parameter
		msghdr msgHdr;
		memset(&msgHdr, 0, sizeof(msgHdr));
		msgHdr.msg_iov = const_cast < iovec * > (reinterpret_cast < const iovec * > (blocks));
		msgHdr.msg_iovlen = blockCount;
		retVal = sendmsg(m_event, &msgHdr, MSG_MORE);
	} else {
		retVal = writev(m_event, reinterpret_cast < const iovec* > (blocks), static_cast < int > (blockCount));
	}
#endif

	size_t bytesWritten;
	if (retVal==0) {
		return retVal;
	} else if (retVal==-1) {
		if ((errno!=EWOULDBLOCK) && (errno!=EAGAIN) && (errno!=EINTR) ) {
			syslog (LOG_ERR, "writev() failed: '%s'", strerror(errno));
			return retVal;
		}
		bytesWritten = 0;
	} else {
		bytesWritten = static_cast < size_t >(retVal);
	}

	if (bytesWritten==completeLength) {
		// we are done!
		return static_cast < ssize_t > (bytesWritten);
	} else {
		// in this case we might have written nothing at all or only a part
		size_t blockSum = 0;
		ssize_t bytesRemaining;

		for( size_t index=0; index<blockCount; ++index) {
			blockSum += blocks[index].size;
			bytesRemaining = blockSum-bytesWritten;
			if (bytesRemaining>0) {
				// this block was not send completely
				size_t start = blocks[index].size-bytesRemaining;
				retVal = sendBlock (static_cast < const unsigned char* > (blocks[index].pData)+start, bytesRemaining, more);
				if (retVal>0) {
					bytesWritten += static_cast < size_t > (retVal);
				} else {
					syslog (LOG_ERR, "Failed to send remaining data of writev: %d '%s'", errno, strerror(errno));
					return -1;
				}
			}
		}
	}

	return static_cast < ssize_t > (bytesWritten);
}



ssize_t hbm::communication::SocketNonblocking::sendBlocks(const dataBlocks_t &blocks, bool more)
{
	std::vector < dataBlock_t > dataBlockVector;
	dataBlockVector.reserve(blocks.size());

	dataBlock_t newIovec;

	for (const dataBlock_t& item: blocks) {
		newIovec.pData = item.pData;
		newIovec.size = item.size;
		dataBlockVector.push_back(newIovec);
	}

	return sendBlocks(&dataBlockVector[0], dataBlockVector.size(), more);
}

ssize_t hbm::communication::SocketNonblocking::sendBlock(const void* pBlock, size_t size, bool more)
{
	const uint8_t* pDat = reinterpret_cast<const uint8_t*>(pBlock);
	size_t BytesLeft = size;
	ssize_t numBytes;
	ssize_t retVal = static_cast < ssize_t > (size);

	struct pollfd pfd;
	pfd.fd = m_event;
	pfd.events = POLLOUT;

	int flags = 0;
	if(more) {
		flags |= MSG_MORE;
	}
	int err;

	while (BytesLeft > 0) {
		numBytes = ::send(m_event, pDat, BytesLeft, flags);
		if(numBytes>0) {
			pDat += numBytes;
			BytesLeft -= static_cast < size_t > (numBytes);
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
			} else if (errno!=EINTR) {
				// a real error happened!
				BytesLeft = 0;
				retVal = -1;
			}
		}
	}
	return retVal;
}


ssize_t hbm::communication::SocketNonblocking::send(const void* pBlock, size_t len, bool more)
{
	int flags = 0;
	if(more) {
		flags |= MSG_MORE;
	}
	return ::send(m_event, pBlock, len, flags);
}


bool hbm::communication::SocketNonblocking::checkSockAddr(const struct ::sockaddr* pCheckSockAddr, socklen_t checkSockAddrLen) const
{
	struct sockaddr sockAddr;
	socklen_t addrLen = sizeof(sockaddr_in);

	char checkHost[256] = "";
	char ckeckService[8] = "";

	char host[256] = "";
	char service[8] = "";
	int err = getnameinfo(pCheckSockAddr, checkSockAddrLen, checkHost, sizeof(checkHost), ckeckService, sizeof(ckeckService), NI_NUMERICHOST | NI_NUMERICSERV);
	if (err != 0) {
		syslog(LOG_ERR, "%s: error from getnameinfo", __FUNCTION__);
		return false;
	}

	if (getpeername(m_event, &sockAddr, &addrLen)!=0) {
		return false;
	}

	if (getnameinfo(&sockAddr, addrLen, host, sizeof(host), service, sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV)!=0) {
		return false;
	}

	if ((strcmp(host, checkHost)==0) && (strcmp(service, ckeckService)==0)
		)
	{
		return true;
	}
	return false;
}

void hbm::communication::SocketNonblocking::disconnect()
{
	if (m_event!=-1) {
		if (::close(m_event)) {
			syslog(LOG_ERR, "closing socket %d failed '%s'", m_event, strerror(errno));
		}
		m_eventLoop.eraseEvent(m_event);
		m_eventLoop.eraseOutEvent(m_event);
	}

	m_event = -1;
}

bool hbm::communication::SocketNonblocking::isFirewire() const
{
	bool retVal = false;

	struct ifreq ifr;
	memset(&ifr, 0, sizeof(struct ifreq));

	if ((::ioctl(m_event, SIOCGIFHWADDR, &ifr, sizeof(struct ifreq))) >= 0) {
		if (ifr.ifr_hwaddr.sa_family == ARPHRD_IEEE1394) {
			retVal = true;
		}
	}
	return retVal;
}
