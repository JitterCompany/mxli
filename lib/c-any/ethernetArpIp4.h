#ifndef __ethernetArpIp4_h
#define __ethernetArpIp4_h

/** @file
 * @brief an ARP for IP4 protocol handler based on the ioFifo-interface.
 */

#include <ioFifo.h>
#include <ethernetArp.h>

MessageResult	ethernetArpIp4Handle(
	const IoFifo *ioFifo,				// for sending response
	const EthernetHeader *ethernetHeader,
	const EthernetAddress *ethernetAddressLocal,
	const Ip4Address *ip4AddressLocal,
	EthernetArpCache *cache,
	Fifo *message
);

#endif

