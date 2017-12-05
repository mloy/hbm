// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <iostream>
#include <stdint.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSTcpIP.h>
#undef max
#undef min

#define __time_t long
#define ssize_t int

#include <algorithm>
#ifndef snprintf
	#define snprintf sprintf_s
#endif


#include "hbm/communication/socketnonblocking.h"


/// Maximum time to wait for connecting
const time_t TIMEOUT_CONNECT_S = 5;

static WSABUF signalBuffer = { 0, NULL };

hbm::communication::SocketNonblocking::SocketNonblocking(sys::EventLoop &eventLoop)
	: m_bufferedReader()
	, m_eventLoop(eventLoop)
	, m_dataHandler()
{
	WORD RequestedSockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	WSAStartup(RequestedSockVersion, &wsaData);
	m_event.completionPort = m_eventLoop.getCompletionPort();
	m_event.overlapped.hEvent = WSACreateEvent();
}

hbm::communication::SocketNonblocking::SocketNonblocking(int fd, sys::EventLoop &eventLoop)
	: m_bufferedReader()
	, m_eventLoop(eventLoop)
	, m_dataHandler()
{
	WORD RequestedSockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	WSAStartup(RequestedSockVersion, &wsaData);
	m_event.completionPort = m_eventLoop.getCompletionPort();
	m_event.overlapped.hEvent = WSACreateEvent();
	m_event.fileHandle = reinterpret_cast < HANDLE > (fd);

	if (setSocketOptions()<0) {
		throw std::runtime_error("error setting socket options");
	}
}

hbm::communication::SocketNonblocking::~SocketNonblocking()
{
	disconnect();
	WSACloseEvent(m_event.overlapped.hEvent);
}

void hbm::communication::SocketNonblocking::setDataCb(DataCb_t dataCb)
{
	DWORD size;
	DWORD flags = 0;

	m_dataHandler = dataCb;
	//m_eventLoop.addEvent(m_event, std::bind(&SocketNonblocking::process, std::ref(*this)));

	// important: Makes io completion to be signalled by the first arriving byte
	WSARecv(reinterpret_cast < SOCKET > (m_event.fileHandle), &signalBuffer, 1, &size, &flags, &m_event.overlapped, NULL);
}

int hbm::communication::SocketNonblocking::setSocketOptions()
{
	bool opt = true;
	int result;

	// switch to non blocking
	u_long value = 1;
	result = ::ioctlsocket(reinterpret_cast < SOCKET > (m_event.fileHandle), FIONBIO, &value);
	if (result == SOCKET_ERROR) {
		return -1;
	}

	// turn off nagle algorithm
	result = setsockopt(reinterpret_cast < SOCKET > (m_event.fileHandle), IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&opt), sizeof(opt));
	if (result == SOCKET_ERROR) {
		return -1;
	}


	// configure keep alive
	DWORD len;
	tcp_keepalive ka;
	ka.keepaliveinterval = 1000; // probe interval in ms
	ka.keepalivetime = 1000; // time of inactivity until first keep alive probe is being send in ms
	// from MSDN: on windows vista and later, the number of probes is set to 10 and can not be changed
	// time until recognition: keepaliveinterval + (keepalivetime*number of probes)
	ka.onoff = 1;
	result = WSAIoctl(reinterpret_cast < SOCKET > (m_event.fileHandle), SIO_KEEPALIVE_VALS, &ka, sizeof(ka), NULL, 0, &len, NULL, NULL);
	if (result == SOCKET_ERROR) {
		return -1;
	}
	return 0;
}

int hbm::communication::SocketNonblocking::connect(const std::string &address, const std::string& port)
{
	struct addrinfo hints;
	struct addrinfo* pResult = NULL;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;


	if( getaddrinfo(address.c_str(), port.c_str(), &hints, &pResult)!=0 ) {
		return -1;
	}
	
	int retVal = connect(pResult->ai_family, pResult->ai_addr, pResult->ai_addrlen);

	freeaddrinfo( pResult );

	return retVal;
}

int hbm::communication::SocketNonblocking::connect(int domain, const struct sockaddr* pSockAddr, socklen_t len)
{

	m_event.fileHandle = reinterpret_cast <HANDLE> (::WSASocket(domain, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED));
	if (m_event.fileHandle == INVALID_HANDLE_VALUE) {
		return -1;
	}

	int err = setSocketOptions();
	if (err < 0) {
		return err;
	}

	err = ::connect(reinterpret_cast < SOCKET > (m_event.fileHandle), pSockAddr, len);
	// success if WSAGetLastError returns WSAEWOULDBLOCK
	if (err == SOCKET_ERROR) {
		if (WSAGetLastError() != WSAEWOULDBLOCK) {
			return -1;
		}

		fd_set fdWrite;

		struct timeval timeout;

		timeout.tv_sec = TIMEOUT_CONNECT_S;
		timeout.tv_usec = 0;


		FD_ZERO(&fdWrite);
		FD_SET(reinterpret_cast < SOCKET > (m_event.fileHandle), &fdWrite);

		err = select(0, NULL, &fdWrite, NULL, &timeout);
		if (err != 1) {
			return -1;
		}
		int value;
		socklen_t len = sizeof(value);
		getsockopt(reinterpret_cast < SOCKET > (m_event.fileHandle), SOL_SOCKET, SO_ERROR, reinterpret_cast < char* > (&value), &len);
		if (value != 0) {
			return -1;
		}
	}

	setDataCb(m_dataHandler);
	return 0;
}

int hbm::communication::SocketNonblocking::process()
{
	if (m_dataHandler) {
		return m_dataHandler(*this);
	} else {
		return -1;
	}
}

ssize_t hbm::communication::SocketNonblocking::receive(void* pBlock, size_t size)
{
  return m_bufferedReader.recv(m_event, pBlock, size);
}

ssize_t hbm::communication::SocketNonblocking::receiveComplete(void* pBlock, size_t len, int msTimeout)
{
	size_t DataToGet = len;
	unsigned char* pDat = static_cast<unsigned char*>(pBlock);
	ssize_t numBytes = 0;


	struct timeval timeVal;
	struct timeval* pTimeVal;
	fd_set recvFds;

	FD_ZERO(&recvFds);
	FD_SET(reinterpret_cast < SOCKET > (m_event.fileHandle), &recvFds);
	int err;

	if(msTimeout>=0) {
		timeVal.tv_sec = msTimeout / 1000;
		int rest = msTimeout % 1000;
		timeVal.tv_usec = rest*1000;
		pTimeVal = &timeVal;
	} else {
		pTimeVal = NULL;
	}



  while (DataToGet > 0) {
    numBytes = m_bufferedReader.recv(m_event, reinterpret_cast<char*>(pDat), static_cast < int >(DataToGet));
    if(numBytes>0) {
      pDat += numBytes;
      DataToGet -= numBytes;
    } else if(numBytes==0) {
      // the peer has performed an orderly shutdown!
      DataToGet = 0;
      return -1;
    } else {
	int lastError = WSAGetLastError();
	if ((lastError == ERROR_IO_PENDING) || (lastError == WSAEWOULDBLOCK)) {
	// wait until there is something to read or something bad happened
	do {
		err = select(0, &recvFds, NULL, NULL, pTimeVal);
	} while((err==-1) && (WSAGetLastError()!=WSAEINTR));
	if(err!=1) {
	  return -1;
	}
      } else {
	return -1;
      }
    }
  }
  return static_cast < ssize_t > (len);
}

ssize_t hbm::communication::SocketNonblocking::sendBlocks(const dataBlock_t *blocks, size_t blockCount)
{
        hbm::communication::dataBlocks_t dataBlocks;
	for(unsigned int i=0; i<blockCount; ++i) {
	        hbm::communication::dataBlock_t dataBlock(blocks[i].pData, blocks[i].size);
		dataBlocks.push_back(dataBlock);
	}
	return sendBlocks(dataBlocks);
}

ssize_t hbm::communication::SocketNonblocking::sendBlocks(const dataBlocks_t &blocks)
{
	std::vector < WSABUF > buffers;
	buffers.reserve(blocks.size());

	size_t completeLength = 0;
	WSABUF newWsaBuf;

	for (dataBlocks_t::const_iterator iter = blocks.begin(); iter != blocks.end(); ++iter) {
		const dataBlock_t& item = *iter;
		newWsaBuf.buf = (CHAR*)item.pData;
		newWsaBuf.len = static_cast < ULONG > (item.size);
		buffers.push_back(newWsaBuf);
		completeLength += item.size;
	}
	DWORD bytesWritten = 0;

	int retVal;
	
	retVal = WSASend(reinterpret_cast < SOCKET > (m_event.fileHandle), &buffers[0], static_cast < DWORD > (buffers.size()), static_cast < LPDWORD > (&bytesWritten), 0, NULL, NULL);
	if (retVal < 0) {
		int retVal = WSAGetLastError();
		if ((retVal != WSAEWOULDBLOCK) && (retVal != ERROR_IO_PENDING) && (retVal != WSAEINTR) && (retVal != WSAEINPROGRESS)) {
			return retVal;
		}
	}

	if (bytesWritten == completeLength) {
		// we are done!
		return bytesWritten;
	} else {
		size_t blockSum = 0;

		for (size_t index = 0; index < buffers.size(); ++index) {
			blockSum += buffers[index].len;
			if (bytesWritten < blockSum) {
				// this block was not send completely
				size_t bytesRemaining = blockSum - bytesWritten;
				size_t start = buffers[index].len - bytesRemaining;
				retVal = sendBlock(buffers[index].buf + start, bytesRemaining, false);
				if (retVal > 0) {
					bytesWritten += retVal;
				}
				else {
					return -1;
				}
			}
		}
	}
	return bytesWritten;
}


ssize_t hbm::communication::SocketNonblocking::sendBlock(const void* pBlock, size_t size, bool more)
{
	const uint8_t* pDat = reinterpret_cast<const uint8_t*>(pBlock);
	size_t BytesLeft = size;
	int numBytes;
	ssize_t retVal = static_cast < ssize_t > (size);

	fd_set recvFds;

	FD_ZERO(&recvFds);
	FD_SET(reinterpret_cast < SOCKET > (m_event.fileHandle), &recvFds);
	int err;

	while (BytesLeft > 0) {
		numBytes = send(reinterpret_cast < SOCKET > (m_event.fileHandle), reinterpret_cast < const char* >(pDat), static_cast < int >(BytesLeft), 0);
		if (numBytes > 0) {
			pDat += numBytes;
			BytesLeft -= numBytes;
		} else if(numBytes==0){
			// connection closed
			BytesLeft = 0;
			retVal = -1;
		} else {
			// -1: error
			int retVal = WSAGetLastError();
			if ((retVal == WSAEWOULDBLOCK) || (retVal == ERROR_IO_PENDING) || (retVal == WSAEINPROGRESS)) {
				err = select(0, NULL, &recvFds, NULL, NULL);
				if (err != 1) {
					BytesLeft = 0;
					retVal = -1;
				}
			} else if (retVal != WSAEINTR) {
				BytesLeft = 0;
				retVal = -1;
			}
		}
	}
	return retVal;
}

void hbm::communication::SocketNonblocking::disconnect()
{
	m_eventLoop.eraseEvent(m_event);
	::closesocket(reinterpret_cast < SOCKET > (m_event.fileHandle));
	m_event.fileHandle = INVALID_HANDLE_VALUE;
}

bool hbm::communication::SocketNonblocking::isFirewire() const
{
	return false;
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
		return false;
	}
	getpeername(reinterpret_cast < SOCKET > (m_event.fileHandle), &sockAddr, &addrLen);
	getnameinfo(&sockAddr, addrLen, host, sizeof(host), service, sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV);
	if (
		(strcmp(host, checkHost) == 0) &&
		(strcmp(service, ckeckService) == 0)
		)
	{
		return true;
	}
	return false;
}
