// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#ifndef _HBM__SYS_NOTIFIER_H
#define _HBM__SYS_NOTIFIER_H

#include "hbm/sys/defines.h"
#include "hbm/exception/exception.hpp"

namespace hbm {
	namespace sys {
		class EventLoop;

		/// Notify someone else waiting for a specific event
		/// If notified n (>0) times until notifier is processed, the callback routine is executed n times!
		class Notifier {
			typedef std::function < void () > Cb_t;
		public:
			/// \throws hbm::exception
			Notifier(EventLoop& eventLoop);

			virtual ~Notifier();

			/// register a callback function for notifications
			/// @param eventHandler Callback function to be executed upon notification
			int set(Cb_t eventHandler);

			/// Eventhandler gets called in event loop context
			int notify();
		private:
			/// must not be copied
			Notifier(const Notifier& op);
			/// must not be assigned
			Notifier operator=(const Notifier& op);

			/// called by eventloop
			int process();

			event m_fd;
			EventLoop& m_eventLoop;
			Cb_t m_eventHandler;
		};
	}
}
#endif
