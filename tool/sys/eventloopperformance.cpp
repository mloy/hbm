#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>

#include "hbm/sys/eventloop.h"
#include "hbm/sys/notifier.h"

static hbm::sys::EventLoop eventloop;
static hbm::sys::Notifier notifier(eventloop);

static size_t EVENTLIMIT = 100000;
static size_t eventCount = 0;

static void notifierCb()
{
	eventCount++;
	if (eventCount>=EVENTLIMIT) {
		eventloop.stop();
	} else {
		notifier.notify();
	}
}

static void notify()
{
	notifier.set(std::bind(&notifierCb));
	notifier.notify();
	std::chrono::high_resolution_clock::time_point t1;
	std::chrono::high_resolution_clock::time_point t2;

	t1 = std::chrono::high_resolution_clock::now();
	eventloop.execute();

	t2 = std::chrono::high_resolution_clock::now();
	std::chrono::microseconds diff = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);

	std::cout << "execution time for " << EVENTLIMIT << " event notifications: " << diff.count() << "µs" << std::endl;
}

static void addRemove()
{
	std::chrono::high_resolution_clock::time_point t1;
	std::chrono::high_resolution_clock::time_point t2;
	t1 = std::chrono::high_resolution_clock::now();
	for (size_t count = 0; count<EVENTLIMIT; ++count) {
		// By creating the notifier it is added to the eventloop. It is removed on destruction.
		hbm::sys::Notifier tempNotifier(eventloop);
	}
	t2 = std::chrono::high_resolution_clock::now();
	std::chrono::microseconds diff = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
	std::cout << "execution time for create/destruct " << EVENTLIMIT << " notifiers: " << diff.count() << "µs" << std::endl;
}

int main()
{
	notify();
	addRemove();
	return EXIT_SUCCESS;
}
