#ifndef __ip4Icmp_h
#define __ip4Icmp_h

/** @file
 * @brief IP4/ICMP low level IO.
 */

#include <ip4.h>

/** ICMP common functionality.
 */
typedef struct __attribute__((packed)) {
	Uint8	type;			///< 8 for echo message, 0 for echo reply
	Uint8	code;			///< may be 0
	Uint16	checksum;		///< 1's complement of 1's complements sum of ICMP-frame
} IcmpHeader;

enum IcmpTypes {
	ICMP_TYPE_ECHO_REPLY			=0,
	ICMP_TYPE_DESTINATION_UNREACHABLE	=3,
	ICMP_TYPE_ECHO_REQUEST			=8,
};

enum IcmpCodes {
	ICMP_CODE_NET_UNREACHABLE		=0,
	ICMP_CODE_HOST_UNREACHABLE		=1,
	ICMP_CODE_PROTOCOL_UNREACHABLE		=2,
	ICMP_CODE_PORT_UNREACHABLE		=3,
	ICMP_CODE_FRAGMENTATION_NEEDED		=4,
	ICMP_CODE_SOURCE_ROUTE_FAILED		=5,
};

/** Prints an ICMP header in human-readble format.
 * @param console output destination
 * @param header the header of the ICMP packet.
 * @return true if fifo buffer was large enough to hold the printed chars.
 */
bool fifoPrintIcmpHeader(Fifo *console, const IcmpHeader *header);
bool fifoPutnetIcmpHeader(Fifo *outputStream, const IcmpHeader *header);
bool fifoGetnetIcmpHeader(Fifo *inputStream, IcmpHeader *header);

typedef struct __attribute__((packed,aligned(2))) {
	Uint16	identifier;		///< used for matching request and reply if code=0, may be 0
	Uint16	sequenceNumber;		///< used for matching request and reply if code=0, may be 0
	Uint8	data[];			///< C99 flexible member
} IcmpEcho;

bool fifoPrintIcmpEcho(Fifo *console, const IcmpEcho *header);
bool fifoPutnetIcmpEcho(Fifo *outputStream, const IcmpEcho *header);
bool fifoGetnetIcmpEcho(Fifo *inputStream, IcmpEcho *header);

/** Calculates an ICMP message header's checksum. The header's checksum must be set to 0.
 * @param icmpHeader the common ICMP header
 * @return the one's complement sum of the message. Set the message's checksum to the complement (~) of
 *   this value to get a valid ICMP checksum.
 */
Uint16 icmpHeaderChecksum(const IcmpHeader* icmpHeader);

/** Calculates an ICMP message header's checksum. The header's checksum must be set to 0.
 * @param icmpHeader the common ICMP header
 * @param icmpEcho the ECHO message specific data
 * @param payload the message body.
 * @return the one's complement sum of the message. Set the message's checksum to the complement (~) of
 *   this value to get a valid ICMP checksum.
 */
Uint16 icmpEchoChecksum(const IcmpHeader* icmpHeader, const IcmpEcho *icmpEcho, Fifo *payload);

typedef struct __attribute__((packed,aligned(2))) {
	Uint32		unused;			///< happily wasting space...
	Ip4Header	ip4Header;		///< original IP header.
	Uint8		data[8];		///< IP options (probably) and 64bits of payload here
} IcmpDestinationUnreachable;

/** Calculates an ICMP message header's checksum. The header's checksum must be set to 0.
 * @param icmpAll the ICMP message including
 * @return the one's complement sum of the message. Set the message's checksum to the complement (~) of
 *   this value to get a valid ICMP checksum.
 */
Uint16 icmpChecksum(Fifo *icmpAll);


#endif

