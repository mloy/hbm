#include <cstdlib>
#include <iostream>
#include <string>

#include <hbm/sys/eventloop.h>
#include <hbm/communication/bufferedreader.h>
#include <hbm/communication/socketnonblocking.h>


int main(int argc, char* argv[])
{
	int result;
	if (argc != 3) {
		std::cout << "syntax: " << argv[0] << " < server address > < server port >" << std::endl;
	}
	hbm::sys::EventLoop eventloop;

	hbm::communication::SocketNonblocking client(eventloop);

	std::string address = argv[1];
	std::string port = argv[2];

	result = client.connect(argv[1], argv[2]);
	if (result!=0) {
		return EXIT_FAILURE;
	}
	const char sndBuffer[] = "hallo";
	char recvBuffer[1024];

	do {
		result = client.sendBlock(sndBuffer, sizeof(sndBuffer), false);
		result = client.receiveComplete(recvBuffer, sizeof(sndBuffer));
	} while (true);

	return EXIT_SUCCESS;
}
