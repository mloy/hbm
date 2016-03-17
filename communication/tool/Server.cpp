#include <iostream>
#include <stdint.h>
#include <string>

#include <hbm/sys/eventloop.h>
#include <hbm/communication/tcpserver.h>
#include <hbm/communication/bufferedreader.h>
#include <hbm/communication/socketnonblocking.h>


static void cb(hbm::communication::workerSocket_t workerSocket)
{
	ssize_t result;
	unsigned char buffer[1024];
	// simply block the server until client closes connection to worker
	do {
		result = workerSocket->receive(buffer, sizeof(buffer));
		if (result <= 0) {
			break;
		}
		workerSocket->sendBlock(buffer, result, false);
	} while (true);
}


void main(int argc, char* argv[])
{
	if (argc != 2) {
		std::cout << "syntax: " << argv[0] << " < server port >" << std::endl;
	}

	hbm::sys::EventLoop eventloop;

	hbm::communication::TcpServer server(eventloop);

	uint16_t port = std::stoul(argv[1]);

	server.start(port, 1, &cb);

	eventloop.execute();
}