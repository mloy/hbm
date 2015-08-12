// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <fstream>
#include <stdexcept>

#include <WinSock2.h>
#undef max
#undef min

#include <cstring>

#include "bufferedreader.h"

namespace hbm {
	BufferedReader::BufferedReader()
		: m_fillLevel(0)
		, m_alreadyRead(0)
	{
	}

	ssize_t BufferedReader::recv(int sockfd, void *buf, size_t desiredLen, int flags)
	{
		// check whether there is something left
		size_t bytesLeft = m_fillLevel - m_alreadyRead;

		if(bytesLeft>=desiredLen) {
			// there is more than or as much as desired
			memcpy(buf, m_buffer+m_alreadyRead, desiredLen);
			m_alreadyRead += desiredLen;
			return static_cast < ssize_t > (desiredLen);
		} else if(bytesLeft>0) {
			// return the rest which is less than desired (a short read)
			memcpy(buf, m_buffer+m_alreadyRead, bytesLeft);
			m_alreadyRead = m_fillLevel;
			return static_cast < ssize_t > (bytesLeft);
		}

		// try to read as much as possible into the buffer
		ssize_t retVal = ::recv(sockfd, (char*)m_buffer, sizeof(m_buffer), flags);

		if(retVal<=0) {
			return retVal;
		}

		m_fillLevel = retVal;

		// length to return is up to the desired length.
		// remember the number of bytes already read.
		if(m_fillLevel>=desiredLen) {
			m_alreadyRead = desiredLen;
		} else {
			m_alreadyRead = m_fillLevel;
		}
		memcpy(buf, m_buffer, m_alreadyRead);
		return static_cast < ssize_t > (m_alreadyRead);
	}
}
