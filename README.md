# What is it for?

The library contains
- Functionality concerning communication and system events often used for system programming for Linux and Micosoft Windows.
- Some convenience classes for string and exception handling.

## Library
Center of all is a library made up from classes from different scopes:

- Communication: Classes concerning network interfaces and communication
- Exceptions: Often used types of exceptions
- String: Common string manipulation methods
- System: Classes concerning system programming (i.e. event loop, timers and notifiers)

Sources are to be found below `lib`. Headers needed to interface with the library are to be found under `include`.



## License

Copyright (c) 2014 Hottinger Baldwin Messtechnik. See the [LICENSE](LICENSE) file for license rights and limitations.

## Prerequisites

### Used Libraries
We try to use as much existing and prooved software as possbile in order to keep implementation and testing effort as low as possible. All libraries used carry a generous license. See the licenses for details.

The open source project jsoncpp is being used as JSON composer and parser. Please make sure to choose Visual Studio 2013 as platform toolset (to be found in the project configuration properties under "General").

The unit tests provided do use the Boost libraries (1.55). Refer to [boost](http://www.boost.org/ "") for details.
For Linux, simply install the Boost development packages of your distribution. For Windows, the projects are tailored to link against the prebuilt boost binaries from [boost](http://www.boost.org/ "").
Download and install the binaries and set the '`BOOST_ROOT`' environment variable to the installation directory.


### Build System
#### Linux
Under Linux the cmake build system is being used. Install it using your distribution package system. 
Create a sub directory inside the project directory. 
Change into this subdirectory and call '`cmake ..`'. Execute '`make`' afterwards
to build all libraries and executables. Finally execute `sudo make install` in order to install the library.


#### Windows
A solution for MSVC2012 is provided.


