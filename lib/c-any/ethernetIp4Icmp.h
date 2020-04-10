#ifndef ethernetIp4Icmp_h
#define ethernetIp4Icmp_h

#include <ip4Icmp.h>
#include <ethernetIp4.h>
#include <ioFifo.h>

/** Handles ICMP echo requests. You must ensure the packet is of type IP/ICMP.
 * @param environment packet sending/receive environment for IP4.
 * @param ethernetHeader the ethernet header of the datagram.
 * @param ip4Header the parsed IP header.
 * @param message the ICMP message contents.
 * @return MESSAGE_DONE if message was handled successfully.
 */
MessageResult ethernetIp4IcmpHandle(
	const EthernetIp4Environment *environment,
	EthernetHeader const *ethernetHeader, Ip4Header const *ip4Header, Fifo *message);

#endif

