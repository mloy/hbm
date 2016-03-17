#include <cstdlib>
#include <cstring>
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
	char buffer;
	// simply block the server until client closes connection to worker
	
	std::cout << "accepted connection!" << std::endl;
	do {
		result = workerSocket->receiveComplete(&buffer, 1);
		if (result == 0) {
			std::cout << "closed connection" << std::endl;
			break;
		} else if (result<0) {
			std::cerr << strerror(errno) << std::endl;
		}
		workerSocket->sendBlock(&buffer, 1, false);
	} while (true);
}


int main(int argc, char* argv[])
{
	if (argc != 2) {
		std::cout << "syntax: " << argv[0] << " < server port >" << std::endl;
		return EXIT_SUCCESS;
	}

	hbm::sys::EventLoop eventloop;

	hbm::communication::TcpServer server(eventloop);

	uint16_t port = std::stoul(argv[1]);

	server.start(port, 1, &cb);

	eventloop.execute();
	return EXIT_SUCCESS;
}
