// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#ifndef _HBM__SYS_DEFINES_H
#define _HBM__SYS_DEFINES_H

#ifdef _WIN32
#include <WinSock2.h>
#ifndef ssize_t
#define ssize_t int
#endif
typedef HANDLE event;
#else
#include <unistd.h>
typedef int event;
#endif
#endif
