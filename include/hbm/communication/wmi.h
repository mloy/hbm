// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#ifndef _WMI_H
#define _WMI_H

#ifdef WIN32

#include "hbm/communication/netadapter.h"

struct IWbemLocator;
struct IWbemServices;


namespace hbm {
	namespace communication {

		// This class provides methods to do some basics WMI queries
		class WMI
		{
		public:
			static long initWMI();
			static void uninitWMI();

			static bool isFirewireAdapter(const communication::Netadapter &adapter);
			static long WMI::enableDHCP(const communication::Netadapter &adapter);
			static long WMI::setManualIpV4(const communication::Netadapter &adapter, const communication::Ipv4Address &manualConfig);

		private:
			static IWbemLocator		*m_pLoc;		// WMI locator
			static IWbemServices	*m_pSvc;		// WMI services
		};
	}
}


#endif // WIN32

#endif // _WMI_H
