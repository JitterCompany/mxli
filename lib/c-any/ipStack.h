#ifndef ipStack_h
#define ipStack_h

/** @file
 * @brief A light IP4 on top of Ethernet, interface enetIoFifo.h .
 *
 * This IP stack processes an incoming packet using predefined handlers plus user-supplied callback handlers.
 * Implemented protocols are: ARP, ICMP echo, ICMP port unreachable, UDP.
 */

#include <ethernet.h>
#include <ethernetArp.h>
#include <ip4.h>
#include <ip4Icmp.h>
#include <ip4Udp.h>
#include <ip4Tcp.h>
#include <fifo.h>
#include <enetIoFifo.h>		// not strictly neccessary at this point

extern volatile int sysTickTimeMs;	///< time base for ARP

/** Configuration of our network device.
 */
typedef struct {
	EthernetAddress	ethernetAddress;
	Ip4Address	ip4Address;
	Ip4Address	gateway;
	int		subnetBits;
	int		ttl;
} Ip4NetworkConfiguration;

typedef enum {
	PACKET_PENDING,		///< handler cannot handle this. Unknown protocol, etc. Pass over to alternate handler.
	PACKET_HANDLED,		///< handler successfully handled packet.
	PACKET_DISCARDED,	///< packet should not be processed any further: checksum error, no need to process,...
	PACKET_RETRY,		///< packet should be delivered to the handler again later.
} InputPacketStatus;

/** A function that handles UDP processing. This function must be supplied by the user.
 * @param input incoming UDP datagram message body. Possibly only a fragment of the body.
 * @param inputAddress the sender's IP-address and port
 * @param port the UDP port targeted
 * @return true, if the packet was processed by the handler, false otherwise (resulting in a port unavailable message.
 */
typedef bool UdpHandler(Fifo *input, int port, const Ip4SocketAddress *inputAddress);

/** A function that handles TCP processing. This function must be supplied by the user.
 * @param input incoming TCP datagram message body. Possibly only a fragment of the body.
 * @param inputAddress the sender's IP-address and port
 * @param port the TCP port targeted
 * @return true, if the packet was processed by the handler, false otherwise (resulting in a port unavailable message.
 */
typedef bool TcpHandler(Fifo *input, const TcpHeader *tcpHeader);

/** Contains the function pointers to user-supplied protocol handlers.
 */
typedef struct {
	UdpHandler *udp;	///< install the user-supplied handler here
	TcpHandler *tcp;	///< install the user-supplied handler here
} Ip4StackHandlers;

/** Provides the information and handlers needed to run the IP stack.
 * Please note, that all pointers must reference objects that are available for the whole runtime of the stack.
 * @param ethernetArpCache an map from IP-addresses to Ethernet-addresses.
 * @param configuration the basic network configuration
 * @param handlers user-defined handlers for protocols - this is where real work is done.
 */
void ipStackInit(
	EthernetArpCache* ethernetArpCache,
	const Ip4NetworkConfiguration *configuration,
	const Ip4StackHandlers *handlers
);

/** Prepares transmission of a UDP packet. At the moment, there's no concurrent transmission of multiple datagrams
 * allowed, so udpSendEnd() must be called soon.
 * @param destination receiver address.
 * @param sourcePort the port we're sending from.
 * @return a Fifo for assembling the UDP data - which is also links to the destination address used int this call.
 */
Fifo* udpSendBegin(const Ip4SocketAddress *destination, Uint16 sourcePort);

/** Allows the current contents of the fifo to be transmitted.
 * The network device is allowed to transmit fragments of the UDP packet.
 * @param fifo the UDP buffer.
 */
void udpSendCommit(Fifo *fifo);

/** Marks the end of the UDP packet. The Fifo contents will be sent as soon as possible.
 * @param fifo the UDP buffer.
 */
void udpSendEnd(Fifo *fifo);

/** Sends an ARP request to resolve an IP-address.
 * @param ip4Address the IP address we're searching an Ethernet address for.
 * @return true if request was sent, false otherwise.
 */
InputPacketStatus ethernetArpRequest(const Ip4Address *ip4Address);

/** Finds out the Ethernet address to use for a given IP address including routing and broadcast.
 * If this function succeeds, the packet can safely be transmitted without further ARP actions.
 * If this function fails, then the host or gateway must first be queried by ARP.
 * @param ip4Address IP destination address.
 * @return the destination Ethernet address to use, 0 if the ARP cache does not (yet) know about the destination.
 */
const EthernetAddress* destinationEthernetAddress(const Ip4Address *ip4Address);

/** This is the basic handler. It examines the Ethernet header, identifies ARP/IP/ICMP/UDP/TCP and calls the
 * handlers for these protocols. In the case of ARP packets, the ARP cache is updated.
 * @param ethernetPacket the whole datagram received from the network, Ethernet CRC removed.
 */
InputPacketStatus handleEthernetPacket(Fifo *ethernetPacket);

#endif

