# Changelog for hbm

## v1.0.1
 - Renamed some netlink event types

## v1.0.2
 - New method for retrieving subnet of ipv4 address/netmask
 - New method checking whether subnet of ipv4 address/netmask is already occupied by another interface
 - Rename class ipv4Address_t => Ipv4Address
 - Rename class ipv6Address_t => Ipv6Address
 - Rename class addressesWithPrefix_t to AddressesWithPrefix
 - Rename class addressesWithNetmask_t to AddressesWithNetmask
 - Rename header communication/ipv4Address_t.h to communication/ipv4Address.h
 - Rename header communication/ipv6Address_t.h to communication/ipv6Address.h

## v1.0.3
 - const correctness for checkSubnet()

## v1.0.4
 - initialize ipv6 prefix on construction

## v1.0.5
 - checksubnet() might exclude a device from tests. This is usefull if the configuration of this interface is to be changed.
