// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <fstream>
#include <stdexcept>
#include <iostream>
#include <string>

#include <WinSock2.h>
#undef max
#undef min

#include <cstring>

#include "hbm/communication/bufferedreader.h"

static WSABUF signalBuffer = { 0, NULL };

namespace hbm {
	namespace communication {
		BufferedReader::BufferedReader()
			: m_fillLevel(0)
			, m_alreadyRead(0)
		{
		}

		ssize_t BufferedReader::recv(hbm::sys::event& ev, void *buf, size_t desiredLen)
		{
			// check whether there is something left
			size_t bytesLeft = m_fillLevel - m_alreadyRead;
			if (bytesLeft >= desiredLen) {
				memcpy(buf, m_buffer + m_alreadyRead, desiredLen);
				m_alreadyRead += desiredLen;
				return static_cast <ssize_t> (desiredLen);
			}
			else if (bytesLeft > 0) {
				// return the rest which is less than desired (a short read)
				memcpy(buf, m_buffer + m_alreadyRead, bytesLeft);
				m_alreadyRead = m_fillLevel;
				return static_cast <ssize_t> (bytesLeft);
			}

			WSABUF buffers[2];
			DWORD Flags = 0;
			DWORD numberOfBytesRecvd;
			buffers[0].buf = reinterpret_cast <CHAR*> (buf);
			buffers[0].len = static_cast <ULONG> (desiredLen);
			buffers[1].buf = reinterpret_cast <CHAR*> (m_buffer);
			buffers[1].len = sizeof(m_buffer);

			int retVal = WSARecv(reinterpret_cast <SOCKET> (ev.fileHandle), buffers, 2, &numberOfBytesRecvd, &Flags, NULL, NULL);
			m_alreadyRead = 0;
			if (retVal < 0) {
				if (WSAGetLastError() == WSAEWOULDBLOCK) {
					// important: Makes io completion to be signalled by the next arriving byte
					Flags = 0;
					if (WSARecv(reinterpret_cast <SOCKET> (ev.fileHandle), &signalBuffer, 1, &numberOfBytesRecvd, &Flags, &ev.overlapped, NULL) == 0) {
						// workaround: if the operation completed with success, there was something to be received. In this case the event will not be signaled. Call WSARecv once more to get the data.
						Flags = 0;
						retVal = WSARecv(reinterpret_cast <SOCKET> (ev.fileHandle), buffers, 2, &numberOfBytesRecvd, &Flags, NULL, NULL);
						if (numberOfBytesRecvd>static_cast < DWORD > (desiredLen)) {
							// WSARecv returns the total number of bytes read
							m_fillLevel = numberOfBytesRecvd - desiredLen;
							return static_cast < ssize_t > (desiredLen);
						}
						m_fillLevel = 0;
						return numberOfBytesRecvd;
					}
				}

				m_fillLevel = 0;
				return retVal;
			}

			if (numberOfBytesRecvd>static_cast < DWORD > (desiredLen)) {
				// WSARecv returns the total number of bytes read
				m_fillLevel = numberOfBytesRecvd - desiredLen;
				return static_cast < ssize_t > (desiredLen);
			}
			m_fillLevel = 0;
			return numberOfBytesRecvd;
		}
	}
}
