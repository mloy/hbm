// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

#include <hbm/string/split.h>

#include "hbm/communication/ipv4Address_t.h"

namespace hbm {
	namespace communication {
		bool ipv4Address_t::equal(const struct ipv4Address_t& op) const
		{
			if( (address==op.address) && (netmask==op.netmask) ) {
				return true;
			}
			return false;
		}

		std::string ipv4Address_t::getSubnet() const
		{
			std::string subnet;
			unsigned int addressByte;
			unsigned int netmaskByte;
			unsigned int subnetByte;
			hbm::string::tokens addressParts = hbm::string::split(address, '.');
			hbm::string::tokens netmaskParts = hbm::string::split(netmask, '.');
			if ((addressParts.size()!=4)||(netmaskParts.size()!=4)) {
				return "";
			}

			for (unsigned int index=0; index<4; ++index) {
				addressByte = std::stoul(addressParts[index], NULL, 10);
				netmaskByte = std::stoul(netmaskParts[index], NULL, 10);
				if ((addressByte>255) || (netmaskByte>255)) {
					// not a valid address!
					return "";
				}
				subnetByte = addressByte & netmaskByte;
				subnet += std::to_string(subnetByte) + '.';
			}
			subnet.pop_back(); // remove trailing '.'
			return subnet;
		}
	}
}

