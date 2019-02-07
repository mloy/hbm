#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include <sys/types.h>
#include <unistd.h>


#include <hbm/sys/eventloop.h>
#include <hbm/communication/socketnonblocking.h>


int main(int argc, char* argv[])
{
	static const unsigned int cycleCount = 10000;
	int retval;
	ssize_t result;
	if (argc < 2) {
		std::cout << "syntax: " << argv[0] << " < server address > [< server port >]" << std::endl;
		std::cout << "syntax: " << argv[0] << " < server unix domain socket >" << std::endl;
		return EXIT_SUCCESS;
	}
	hbm::sys::EventLoop eventloop;

	hbm::communication::SocketNonblocking client(eventloop);

	if (argc>=3) {
		std::string port = argv[2];
		std::string address = argv[1];

		retval = client.connect(address, port);
	} else {
		std::string path = argv[1];
		retval = client.connect(path);
	}

	if (retval!=0) {
		std::cerr << "could not connect to server!" << std::endl;
		return EXIT_FAILURE;
	}

	__pid_t processId = getpid();
	char sndBuffer[128];
	snprintf(sndBuffer, sizeof(sndBuffer), "hallo (%u)", processId);
	char recvBuffer[1024];

	std::chrono::high_resolution_clock::time_point t1;
	std::chrono::high_resolution_clock::time_point t2;

	t1 = std::chrono::high_resolution_clock::now();
	for(unsigned int cycle=0; cycle<cycleCount; ++cycle) {
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
	}
	t2 = std::chrono::high_resolution_clock::now();
	std::chrono::microseconds diff = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
	std::cout << "average time (" << cycleCount << " cycles) for send/receive: " << diff.count()/cycleCount << "µs" << std::endl;


	return EXIT_SUCCESS;
}
