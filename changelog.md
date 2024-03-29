# Changelog for hbm

## v1.0.39
 - hbm::communication::Netadapter::getIpv4DefaultGateway() returns an empty string when there is no default gateway

## v1.0.38
 - Fix and simplify cmake find-package support

## v1.0.37
 - Suport cmake find-package
 - More C++11 for efficiency
 - Increased test coverage

## v1.0.34
 - Linux: More efficient eventloop handling
 - Linux: Minor optimization of timer and notifier

## v1.0.33
 - Windows: socket shutdown after removal from event loop

## v1.0.32
 - Windows: shutdown() before closing socket in order to force gracefull shutdown

## v1.0.31
 - Use C++ range loops
 - Windows: Not compatible with Windows XP anymore. We use windows platforms sdk 8.1 and windows platform toolset v1.41
 - Some C++11 optimization

## v1.0.30
 - Windows: netadapterlist may not contain devices without an address under windows

## v1.0.29
 - Windows: Fix multicast issues

## v1.0.28
 - Linux: Fix getting socket option for determination of socket domain

## v1.0.27
 - Linux: Fix UNIX domain sockets compatibility issue in server class

## v1.0.26 
 - Linux: Support for UNIX domain sockets

## v1.0.25
 - Linux: Fix versioning and interface header issues

## v1.0.24
 - Linux: Identify interface for sending multicast by device index if possible. 
   This even works if the interface does not have an ip address.

## v1.0.23
 - Windows: Fix notifier and timer

## v1.0.22
 - Linux Optimization: It is sufficient to read once in order to rearm timerfd and eventfd.

## v1.0.19
 - Linux: Allow gathered write on socket to use cork option

## v1.0.17
 - Linux: Notifier collects the number of events first, executes callback accordingly 
 and returns 0 so that the eventloop won't call
 again.
 - Linux: Timer does not log if event happens more than once. This takes a serious 
 amount of time which is disturbing in many cases.

## v1.0.16
 - Linux: Timer callback function is to be executed once! We read all events from the
 timer until would block, execute once and return 0 so that the eventloop won't call 
 again.

## v1.0.15
 - Fair eventloop under linux: The callback of each signaled event is called only once.
 After all the callbacks of all signaled events were called, we start from the 
 beginning until no signaled event is left.

## v1.0.14
 - Fix IOCP bug under Windows:	We use the completionkey to determine the event to handle.
 Using the ovelapped structure is dangearous because it is owned by the object that 
 registers the event.
 After destruction, GetQueuedCompletionStatus() might deliver an overlapped structure 
 that does not exist anymore!

## v1.0.12, v1.0.13
 - Build system issues

## v1.0.11
 - fix bug in eventloop (Linux): callback function might destroy the object that
   triggered the event.
 - ipv4 address ranges reserved for QuantumX FireWire should not be checked here
 - Linux eventloop: Fix confusion when removing in or out event and keep the other

## v1.0.10
 - Build system issues

## v1.0.9
 - Update MSVC projects to visual studio 2017

## v1.0.8
 - hbm::jsonrpc_exception does no longer depend on jsoncpp

## v1.0.7
 - windows: tcpserver does not support ipv6
 - socketnonblocking: introduced writable event under linux (setOutDataCb())

## v1.0.6
 - multicastserver: Work with interface index instead of interface name
 - netlink: Fix recognotion of adapter going down
 - lib, test, tool: Find jsoncpp using pkg-config provided by jsoncpp
 - doxygen documentation is generated automatically

## v1.0.5
 - isFirewireAdapter() works under Windows
 - new method getIpv4MappedIpv6Address returns ipv4 address mapped in ipv6 address
 - accept upper case letters in ipv6 address

## v1.0.4
 - initialize ipv6 prefix on construction
 - checksubnet() might exclude a device from tests. This is usefull if the configuration of this interface is to be changed.

## v1.0.3
 - const correctness for checkSubnet()

## v1.0.2
 - New method for retrieving subnet of ipv4 address/netmask
 - New method checking whether subnet of ipv4 address/netmask is already occupied by another interface
 - Rename class ipv4Address_t => Ipv4Address
 - Rename class ipv6Address_t => Ipv6Address
 - Rename class addressesWithPrefix_t to AddressesWithPrefix
 - Rename class addressesWithNetmask_t to AddressesWithNetmask
 - Rename header communication/ipv4Address_t.h to communication/ipv4Address.h
 - Rename header communication/ipv6Address_t.h to communication/ipv6Address.h

## v1.0.1
 - Renamed some netlink event types
