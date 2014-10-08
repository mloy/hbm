/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2007 Hottinger Baldwin Messtechnik GmbH
 * Im Tiefen See 45
 * 64293 Darmstadt
 * Germany
 * http://www.hbm.com
 * All rights reserved
 *
 * The copyright to the computer program(s) herein is the property of
 * Hottinger Baldwin Messtechnik GmbH (HBM), Germany. The program(s)
 * may be used and/or copied only with the written permission of HBM
 * or in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 * This copyright notice must not be removed.
 *
 * This Software is licenced by the
 * "General supply and license conditions for software"
 * which is part of the standard terms and conditions of sale from HBM.
*/


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <stdexcept>

#include <sys/types.h>


#ifdef _WIN32
#define syslog fprintf
#define LOG_DEBUG stdout
#define LOG_INFO stdout
#define LOG_ERR stderr
#define popen(a, b) _popen(a, b)
#define pclose(a) _pclose(a)
#else
#include <wait.h>
#include <syslog.h>
#include <unistd.h>
#endif

#include <errno.h>

#include "hbm/exception/exception.hpp"
#include "hbm/sys/executecommand.h"

namespace hbm {
	namespace sys {
		void executeCommand(const std::string& command)
		{
		#ifdef _STANDARD_HARDWARE
			std::cout << command << std::endl;
		#else
			FILE* f = popen(command.c_str(),"r");
			if (f == NULL) {
				std::string msg = std::string(__FUNCTION__) + "popen failed (cmd=" + command + ")!";
				throw hbm::exception::exception(msg);
			} else {
				pclose(f);
			}
		#endif
		}

		int executeCommand(const std::string& command, const std::string& param, const std::string& stdinString)
		{
		#ifdef _STANDARD_HARDWARE
			std::cout << command << " " << param << " < " << stdinString << std::endl;
		#else
			static const unsigned int PIPE_READ = 0;
			static const unsigned int PIPE_WRITE = 1;
			int pfd[2];
			pid_t cpid;

			if (pipe(pfd) == -1){
				syslog(LOG_ERR, "error creating pipe");
				return -1;
			}
			cpid = fork();
			if (cpid == -1) {
				syslog(LOG_ERR, "error forking process");
				return -1;
			}
			if (cpid == 0) {
				// Child
				int result;
				close(pfd[PIPE_WRITE]); // close unused write end

				// redirect stdin
				if (dup2(pfd[PIPE_READ], STDIN_FILENO) == -1) {
					syslog(LOG_ERR, "error redirecting stdin");
					return -1;
				}
				//syslog(LOG_INFO, "execute: %s %s", command.c_str(), param.c_str());
				result = execl(command.c_str(), command.c_str(), param.c_str(), static_cast<char*>(NULL));
				// if we get here at all, an error occurred, but we are in the child
				// process, so just exit
				syslog(LOG_ERR, "error executing %s '%s'", command.c_str(), strerror(errno));
				exit(result);
			} else if ( cpid > 0 ) {
				// Parent
				close(pfd[PIPE_READ]); // close unused read end

				// send data to stdin of child
				//syslog(LOG_INFO, "piping data to stdin: '%s'", stdin.c_str());
				write(pfd[PIPE_WRITE], stdinString.c_str(), stdinString.size());
				close(pfd[PIPE_WRITE]);

				//syslog(LOG_INFO, "wait for children...");
				// wait for child to finish
				wait(NULL);
				//syslog(LOG_INFO, "done!");
			} else {
				//syslog(LOG_ERR, "failed to create child!");
				close(pfd[PIPE_READ]);
				close(pfd[PIPE_WRITE]);
			}
		#endif
			return 0;
		}

		std::string executeCommandWithAnswer(const std::string& command)
		{
			std::string retVal;
		#ifdef _STANDARD_HARDWARE
			std::cout << command << std::endl;
		#else
			FILE* f = popen(command.c_str(),"r");
			if (f == NULL) {
				std::string msg = std::string(__FUNCTION__) + "popen failed (cmd=" + command + ")!";
				throw hbm::exception::exception(msg);
			} else {
				char buffer[1024] = {'\0'};
				fread(buffer, sizeof(buffer), 1, f);
				retVal = buffer;
				pclose(f);
			}
			#endif

			return retVal;
		}
	}
}

