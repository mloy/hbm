// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#undef max
#undef min
#endif

#include <memory>
#include <cstring>

#include "Ws2ipdef.h"

#include <errno.h>

#include "hbm/communication/socketnonblocking.h"
#include "hbm/communication/tcpserver.h"
#include "hbm/sys/eventloop.h"


namespace hbm {
	namespace communication {
		TcpServer::TcpServer(sys::EventLoop &eventLoop)
			: m_acceptSocket(INVALID_SOCKET)
			, m_eventLoop(eventLoop)
			, m_acceptCb()
		{
			WORD RequestedSockVersion = MAKEWORD(2, 2);
			WSADATA wsaData;
			WSAStartup(RequestedSockVersion, &wsaData);

			m_listeningEvent.completionPort = m_eventLoop.getCompletionPort();
			m_listeningEvent.overlapped.hEvent = WSACreateEvent();
		}

		TcpServer::~TcpServer()
		{
			stop();
			WSACloseEvent(m_listeningEvent.overlapped.hEvent);
		}

		int TcpServer::start(uint16_t port, int backlog, Cb_t acceptCb)
		{
			m_listeningEvent.fileHandle = reinterpret_cast < HANDLE > (socket(AF_INET, SOCK_STREAM, 0));

			DWORD winLen;

			GUID InBuffer[] = WSAID_ACCEPTEX;
			int status = WSAIoctl(reinterpret_cast < SOCKET > (m_listeningEvent.fileHandle), SIO_GET_EXTENSION_FUNCTION_POINTER, &InBuffer, sizeof(InBuffer), &m_acceptEx, sizeof(m_acceptEx), &winLen, NULL, NULL);
			if (status != 0) {
				return -1;
			}

			SOCKADDR_IN address;
			memset(&address, 0, sizeof(address));
			address.sin_family = AF_INET;
			address.sin_addr.s_addr = htonl(INADDR_ANY);
			address.sin_port = htons(port);	

			if (::bind(reinterpret_cast < SOCKET > (m_listeningEvent.fileHandle), reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
				printf("%s: Binding socket to port initialization failed '%s'", __FUNCTION__, strerror(errno));
				return -1;
			}

			// switch to non blocking
			u_long value = 1;
			if (::ioctlsocket(reinterpret_cast <SOCKET> (m_listeningEvent.fileHandle), FIONBIO, &value) == -1)
			{
				return -1;
			}

			m_acceptCb = acceptCb;

			if (listen(reinterpret_cast < SOCKET > (m_listeningEvent.fileHandle), backlog) == -1) {
				return -1;
			}

			if (prepareAccept() == -1) {
				return -1;
			}

			return m_eventLoop.addEvent(m_listeningEvent, std::bind(&TcpServer::process, std::ref(*this)));
		}

		void TcpServer::stop()
		{
			m_eventLoop.eraseEvent(m_listeningEvent);
			closesocket(reinterpret_cast < SOCKET > (m_listeningEvent.fileHandle));
		}

		clientSocket_t TcpServer::acceptClient()
		{
			return clientSocket_t(new SocketNonblocking(m_acceptSocket, m_eventLoop));
		}


		int TcpServer::process()
		{
			clientSocket_t worker = acceptClient();
			if (!worker) {
				return 0;
			}

			if (m_acceptCb) {
				m_acceptCb(std::move(worker));
			}
			prepareAccept();
			return 0;
		}

		int TcpServer::prepareAccept()
		{
			m_acceptSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

			// We do not want to receive data here hence dwReceiveDataLength is 0.
			// From MSDN for dwLocalAddressLength, dwRemoteAddressLength:
			// "This value must be at least 16 bytes more than the maximum address length for the transport protocol in use."
			BOOL result = m_acceptEx(reinterpret_cast < SOCKET > (m_listeningEvent.fileHandle), m_acceptSocket, m_acceptBuffer, 0, sizeof(sockaddr_in6)+16, sizeof(sockaddr_in6)+16, &m_acceptSize, &m_listeningEvent.overlapped);
			if (result == FALSE) {
				if (WSAGetLastError() == WSA_IO_PENDING) {
					return 0;
				}
				else {
					return -1;
				}
			}

			return 0;

		}
	}
}
