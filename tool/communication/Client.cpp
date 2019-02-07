#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include <hbm/sys/eventloop.h>
#include <hbm/communication/socketnonblocking.h>


int main(int argc, char* argv[])
{
	int retval;
	ssize_t result;
	if (argc < 2) {
		std::cout << "syntax: " << argv[0] << " < server address > [< server port >]" << std::endl;
		std::cout << "syntax: " << argv[0] << " < server unix domain socket >" << std::endl;
	}
	hbm::sys::EventLoop eventloop;

	hbm::communication::SocketNonblocking client(eventloop);

	std::string port;
	std::string address = argv[1];
	if (argc>=3) {
		port = argv[2];
	}

	retval = client.connect(address, port);
	if (retval!=0) {
		std::cerr << "could not connect to server!" << std::endl;
		return EXIT_FAILURE;
	}
	const char sndBuffer[] = "hallo";
	char recvBuffer[1024];

	do {
		result = client.sendBlock(sndBuffer, sizeof(sndBuffer), false);
		if (result != sizeof(sndBuffer)) {
			std::cerr << "could not send complete data! " << result << " of " << sizeof(sndBuffer) << " bytes send" << std::endl;
			return EXIT_FAILURE;
		}

		result = client.receiveComplete(recvBuffer, sizeof(sndBuffer));
		if (result != sizeof(sndBuffer)) {
			std::cerr << "received unexpected amount of data! " << result << " of " << sizeof(sndBuffer) << " bytes received " << std::endl;
			return EXIT_FAILURE;
		}
		if (strncmp(recvBuffer, sndBuffer, sizeof(sndBuffer))) {
			std::cerr << "received unexpected data! " << recvBuffer << "!=" << sndBuffer << std::endl;
		}



	} while (true);

	return EXIT_SUCCESS;
}
