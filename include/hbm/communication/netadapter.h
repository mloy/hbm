// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#ifndef _NETADAPTER_H
#define _NETADAPTER_H

#include <deque>
#include <string>
#include <stdint.h>

#include <hbm/communication/ipv4address.h>
#include <hbm/communication/ipv6address.h>

namespace hbm {
	namespace communication {
		enum enResult {
			ERR_SUCCESS = 0,
			ERR_NO_SUCCESS = -1,
			ERR_INVALIDADAPTER = -2,
			ERR_INVALIDIPADDRESS = -3,
			ERR_INVALIDCONFIGMETHOD = -4,
			ERR_ADAPTERISDOWN = -5,
			WARN_RESTART_REQUIRED = 4,
			WARN_INVALIDCONFIGMETHOD = 5
		};

		// we use a double ended queue here because we might insert to the front or to the back.
		typedef std::deque < Ipv6Address > AddressesWithPrefix;
		typedef std::deque < Ipv4Address > AddressesWithNetmask;


		/// Everything about a network adapter
		class Netadapter
		{
		public:


			Netadapter();

			std::string getName() const { return m_name; }

			const AddressesWithNetmask& getIpv4Addresses() const
			{
				return m_ipv4Addresses;
			}

			const AddressesWithPrefix& getIpv6Addresses() const
			{
				return m_ipv6Addresses;
			}

			/// \return formatted MAC address in HEX punctuated with ":" and upper case letters
			std::string getMacAddressString() const { return m_macAddress; }

			/// \return 0 for non FireWire adapters
			uint64_t getFwGuid() const
			{
				return m_fwGuid;
			}

			unsigned int getIndex() const
			{
				return m_index;
			}

			/// \return true if this is a firewire adapter
			bool isFirewireAdapter() const;

			/// If interfaces are configured using DHCP, another default gateway might be used.
			/// \return Address of the manual ipv4 default gateway
			static std::string getIpv4DefaultGateway();

			std::string m_name;

			AddressesWithNetmask m_ipv4Addresses;
			AddressesWithPrefix m_ipv6Addresses;

			std::string m_macAddress;

			uint64_t m_fwGuid;

			/// interface index
			unsigned int m_index;
		};
	}
}
#endif
