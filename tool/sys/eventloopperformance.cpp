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

int main()
{
	notifier.set(std::bind(&notifierCb));
	notifier.notify();
	eventloop.execute();
	return EXIT_SUCCESS;
}
