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

#ifndef _NETLINK_H
#define _NETLINK_H

#include "hbm/exception/exception.hpp"
#include "hbm/communication/netadapterlist.h"
#include "hbm/communication/multicastserver.h"

namespace hbm {
	class Netlink {
	public:
		/// \throws hbm::exception
		Netlink();
		virtual ~Netlink();

		/// receive events from netlink. Adapt netadapter list and mulicast server accordingly
		ssize_t receiveAndProcess(communication::NetadapterList &netadapterlist, communication::MulticastServer &mcs) const;

		/// to poll
		int getFd() const
		{
			return m_fd;
		}

		int stop();

	private:
		ssize_t receive(char* pReadBuffer, size_t bufferSize) const;
		/// \param[in, out] netadapterlist will be adapted when processing netlink events
		/// \param[in, out] mcs will be adapted when processing netlink events
		void process(char *pReadBuffer, size_t bufferSize, communication::NetadapterList &netadapterlist, communication::MulticastServer &mcs) const;
		int m_fd;
	};
}

#endif
