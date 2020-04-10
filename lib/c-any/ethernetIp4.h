#ifndef __ethernetIp4_h
#define __ethernetIp4_h

/** @file
 * @brief Definitions suitable for running IP over Ethernet.
 */
#include <ethernetArp.h>
#include <ip4.h>
#include <ioFifo.h>

/** Configuration of our network device.
 */
typedef struct {
	EthernetAddress	ethernetAddress;
	Ip4Address	ip4Address;
	Ip4Address	gateway;
	int		subnetBits;
	int		ttl;
} EthernetIp4Configuration;

typedef struct {
	const EthernetIp4Configuration	*configuration;
	EthernetArpCache		*arpCache;
	const IoFifo			*ioFifo;
} EthernetIp4Environment;

#endif
