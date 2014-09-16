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


/** @file */

#ifndef _HBM__NETADAPTERLIST_H
#define _HBM__NETADAPTERLIST_H

#include <vector>
#include <map>
#include <string>

#include <boost/thread/mutex.hpp>

#include "netadapter.h"

namespace hbm {
	namespace communication {
		/// Informationen ueber alle verfuegbaren IP-Schnittstellen.
		class NetadapterList
		{
		public:
			/// interface index is the key
			typedef std::map < unsigned int, Netadapter > tAdapters;
			typedef std::vector < Netadapter > tAdapterArray;

			NetadapterList();

			tAdapters get() const;

			/// the same order as returned by get()
			tAdapterArray getArray() const;

			/// \throws hbm::exception
			Netadapter getAdapterByName(const std::string& adapterName) const;

			/// get adapter by interface index
			/// \throws hbm::exception
			Netadapter getAdapterByInterfaceIndex(unsigned int interfaceIndex) const;

			void update();

		private:

			void enumAdapters();

			tAdapters m_adapters;
			mutable boost::mutex m_adaptersMtx;
		};
	}
}
#endif
