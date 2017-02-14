// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#ifndef _NETADAPTER_H
#define _NETADAPTER_H

#include <vector>
#include <deque>
#include <string>
#include <stdint.h>

#include <hbm/communication/ipv4Address_t.h>
#include <hbm/communication/ipv6Address_t.h>

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

		typedef std::vector < ipv6Address_t > addressesWithPrefix_t;
		// we use a double ended queue here because we might insert to the front or to the back.
		typedef std::deque < ipv4Address_t > addressesWithNetmask_t;


		class Netadapter
		{
		public:


			Netadapter();

			std::string getName() const { return m_name; }

			const addressesWithNetmask_t& getIpv4Addresses() const
			{
				return m_ipv4Addresses;
			}

			const addressesWithPrefix_t& getIpv6Addresses() const
			{
				return m_ipv6Addresses;
			}

			/// returns formatted MAC address in HEX punctuated with ":" and upper case letters
			std::string getMacAddressString() const { return m_macAddress; }

			/// returns 0 for non FireWire adapters
			uint64_t getFwGuid() const
			{
				return m_fwGuid;
			}

			unsigned int getIndex() const
			{
				return m_index;
			}

			/// \warning not supported under windows (returns false always)
			bool isFirewireAdapter() const
			{
				if (m_fwGuid) {
					return true;
				} else {
					return false;
				}
			}

			static std::string getIpv4DefaultGateway();

			static bool isApipaAddress(const std::string& address);
			static bool isIpv6LinkLocalAddress(const std::string& address);
			static bool isValidManualIpv4Address(const std::string& ip);
			static bool isValidIpv4Netmask(const std::string& ip);
			static int getPrefixFromIpv4Netmask(const std::string& netmask);
			static std::string getIpv4NetmaskFromPrefix(unsigned int prefix);

			std::string m_name;

			addressesWithNetmask_t m_ipv4Addresses;
			addressesWithPrefix_t m_ipv6Addresses;

			std::string m_macAddress;

			uint64_t m_fwGuid;

			unsigned int m_index; // interface index retrieved by ::GetAdaptersInfo
		};
	}
}
#endif
