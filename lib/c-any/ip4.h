#ifndef __ip4_h
#define __ip4_h

/** @file
 * @brief IP version 4 and subprotocols.
 *
 * Defines the IP header (without flags so far) and the subprotocols needed to run UDP.
 */

#include <stdbool.h>
#include <integers.h>
#include <fifo.h>

/** IP version 4 address in network byte order.
 */
typedef union {
	Uint8		int8s[4];
	Uint32		int32;		///< access as a single integer
} Ip4Address;

/** Ip4 broadcast address 255.255.255.255 .
 */
extern Ip4Address IP4_ADDRESS_BROADCAST;

/** Checks if an address lies within a subnet.
 * @param subnet the subnet IP address or any other IP address within that subnet
 * @param subnetBits the number of 0-bits in the netmask. if net = 192.168.0.0/8 then netmaskBits=8
 * @param address the address to check for subnet membership.
 */
static inline bool ip4IsSubnet(const Ip4Address *subnet, int subnetBits, const Ip4Address *address) {
	return subnet->int32<<subnetBits == address->int32<<subnetBits;
}

static inline bool ip4IsBroadcast(const Ip4Address *address) {
	return address->int32 == IP4_ADDRESS_BROADCAST.int32;
}

static inline bool ip4IsEqual(const Ip4Address *a1, const Ip4Address *a2) {
	return a1->int32==a2->int32;
}

/** Converts an IPv4 address into a human-readable string called dotted decimal.
 * @param fifo the string destination.
 * @param ip4Address the IPv4 address as an array.
 * @return true in case of success, false if formatBuffer is exhausted.
 */
bool fifoPrintIp4Address(Fifo *fifo, const Ip4Address* ip4Address);

/** Parses an IPv4 address from a character string.
 * @param fifo a Fifo containing the character string. An IPv4 address looks like this:
 *   192.168.0.1 (called dotted decimal).
 * @param ip4Address the result value (an array - always passed as pointer in C).
 * @return true if successful, false otherwiese (and iterator is restored).
 */
bool fifoParseIp4Address(Fifo *fifo, Ip4Address* ip4Address);

/** Writes an IPv4 address into a binary stream in network byte order.
 * @return true in case of success, false if formatBuffer is exhausted.
 */
static inline bool fifoPutnetIp4Address(Fifo *fifo, const Ip4Address* ip4Address) {
	return fifoPutN(fifo,ip4Address,sizeof *ip4Address);
}

/** Reads an IPv4 address from binary stream in network byte order.
 * @param fifo input stream
 * @param ip4Address the result value (an array - always passed as pointer in C).
 * @return true if successful, false otherwiese (and iterator is restored).
 */
static inline bool fifoGetnetIp4Address(Fifo *fifo, Ip4Address* ip4Address) {
	return fifoGetN(fifo,ip4Address,sizeof *ip4Address);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IP header

/** IPv4 packet in little endian format - you cannot put this directly on the wire. Use fifoPutXX-functions for this.
 */
typedef struct __attribute__((packed,aligned(2))) {
	union {
		Uint8	versionAndHeaderLength4;
		struct __attribute__((packed)) { Uint8
			headerLength4:4,	///< header length, unit 4bytes; 5 (no options) or greater
			version:4;		///< IP version; should be 4
		};
	};
	Uint8		tos;		///< TOS, type of service.
	Uint16		totalLength;	///< TL, total length 0..65536
	Uint16		identification;	///< indentication for reassembly of fragmented packages
	union {
		struct __attribute__((packed)) { Uint16	// ill formed big-endian bit packing
			fragmentOffset8:13,	///< fragment offset in steps of 8 bytes. TESTED bit pos
			mf:1,			///< more fragments; zero for last/only fragment. TESTED bit pos
			df:1,			///< don't fragment flag. TESTED bit pos
			reserved:1;		///< should be set to zero. TESTED bit pos
		};
		Uint16	flagsAndFragmentOffset8;	///< this is big-endian encoded
	};
	Uint8		ttl;		///< time to live, 64 typically
	Uint8		protocol;	///< IP subprotocol: 1=ICMP, 2=IGMP, 6=TCP, 17=UDP, ...
	Uint16		headerChecksum;	///< big endian, 1's complement 16bit sum; checksum yields 0xFFFF for val. hdr
	Ip4Address	sourceAddress;
	Ip4Address	destinationAddress;
} Ip4Header;

/** IP subprotocols.
 */
enum Ip4Protocols {
	IP_PROTOCOL_ICMP	=1,
	IP_PROTOCOL_IGMP	=2,
	IP_PROTOCOL_TCP		=6,
	IP_PROTOCOL_UDP		=17,
};

/** Calculates the one's complement checksum used in IP. It's a 16bit checksum of big endian 16bit values.
 * @param packet the data for checksum calculation.
 * @return the checksum in little endian format.
 */
Uint16 fifoCalculateOnesComplementSumBigEndian(Fifo *packet);

/** Calculates IP4 header checksum. The header's checksum field must be set to 0 for this calculation.
 * Set the IP4 header's checksum to ~result.
 * @param header the IP4 header without options - this function cannot handle options.
 */
Uint16 ip4HeaderChecksum(const Ip4Header *header);

/** Prints a IP frame's header in human-readble form.
 * @param fifo output destination
 * @param header at least the header of a IP frame.
 * @return true if fifo buffer was large enough to hold the printed chars.
 */
bool fifoPrintIp4Header(Fifo *fifo, const Ip4Header *header);

/** Writes an IP4 header into a binary stream in network byte order. IP options may be written afterwards but have to be included
 * in the checksum.
 * @param fifo the output stream
 * @param header the IP4 header with correct checksum field.
 * @return true if the header fit into the stream, false otherwise (and fifo unchanged).
 */
bool fifoPutnetIp4Header(Fifo *fifo, const Ip4Header *header);

/** Reads an IP4 header from a binary stream in network byte order. IP options are detected and removed from the
 * stream but they are NOT recorded or processed.
 * @param networkStream the input stream.
 * @param header destination memory
 * @return true in case of successful extraction, false otherwise (and fifo unchanged).
 */
bool fifoGetnetIp4Header(Fifo *networkStream, Ip4Header *header);

////////////////////////////////////////////////////////////////////////////////////////////////////
//

/** An IP configuration for a single network interface.
  */
typedef struct {
	Ip4Address	addressMe;
	Ip4Address	addressGateway;
	Uint8		subnetBits;
	Uint8		ttl;
} Ip4Configuration;

/** Identifier for a remote process.
 */
struct Ip4SocketAddress {
	Ip4Address	ip4Address;	///< Remote network device address
	Uint16		port;		///< Remote process address; little endian (host byte order).
};
typedef struct Ip4SocketAddress Ip4SocketAddress;

/** Prints a socket address.
 * @param fifo output destination
 * @param socketAddress the IP-address/port to print
 * @return true if fifo buffer was large enough to hold the printed chars.
 */
bool fifoPrintIp4SocketAddress(Fifo *fifo, const Ip4SocketAddress* socketAddress);

#endif

