// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#ifndef _HBM__NETADAPTERLIST_H
#define _HBM__NETADAPTERLIST_H

#include <vector>
#include <map>
#include <string>
#include <mutex>

#include "netadapter.h"

namespace hbm {
	namespace communication {
		/// Informationen ueber alle verfuegbaren IP-Schnittstellen.
		class NetadapterList
		{
		public:
			/// interface index is the key
			typedef std::map < unsigned int, Netadapter > Adapters;
			typedef std::vector < Netadapter > AdapterArray;

			NetadapterList();

			Adapters get() const;

			/// the same order as returned by get()
			AdapterArray getArray() const;

			/// \throws hbm::exception
			Netadapter getAdapterByName(const std::string& adapterName) const;

			/// get adapter by interface index
			/// \throws hbm::exception
			Netadapter getAdapterByInterfaceIndex(unsigned int interfaceIndex) const;

			/// check whether subnet of requested address is already occupied by an address of an interface
			/// \param excludeAdapterName Name of an adapter to exclude from check. This is usefull to allow collision on this adapter before changing its address. Leave empty if no interface is to be excluded.
			/// \return name of the occupying interface or an empty string
			std::string checkSubnet(const std::string& excludeAdapterName, const communication::Ipv4Address& requestedAddress) const;

			void update();

		private:

			void enumAdapters();

			Adapters m_adapters;
			mutable std::mutex m_adaptersMtx;
		};
	}
}
#endif
