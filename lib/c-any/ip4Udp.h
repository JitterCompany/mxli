#ifndef __ip4Udp_h
#define __ip4Udp_h

/** @file
 * @brief UDP-related low-level IO functions.
 */

#include <ip4.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// UDP

/** UDP header in little endian format - you cannot put this directly on the wire.
 */
typedef struct __attribute__((packed,aligned(2))) {
	Uint16		source;			///< source port
	Uint16		destination;		///< destination port
	Uint16		length;			///< UDP header + payload
	Uint16		checksum;		///< 1's complement sum of Pseudoheader and UDP packet.
} UdpHeader;

/** UDP pseudo header in little endian format - you cannot put this directly on the wire.
 */
typedef struct __attribute__((packed,aligned(2))) {
	Ip4Address	sourceAddress;
	Ip4Address	destinationAddress;
	Uint8		mbz;
	Uint8		protocol;
	Uint16		udpLength;
} UdpPseudoHeader;

/** Calculates the UDP header checksum that follows the pseudo-header (conceptionally).
 */
Uint16 udpHeaderChecksum(UdpHeader const* header);

/** Calculates the UDP pseudo-header checksum used in the UDP header.
 */
Uint16 udpPseudoHeaderChecksum(UdpPseudoHeader const *pseudoHeader);

/** Prints an UDP-header in human-readable form.
 */
bool fifoPrintUdpHeader(Fifo *console, const UdpHeader *udpHeader);

/** Writes a UDP header in network byte order.
 * @param outputStream a fifo the bytes of which can be put directly on the wire.
 * @param udpHeader little endian byte order UDP header.
 * @return true if no buffer overflow occurred, false otherwise.
 */
bool fifoPutnetUdpHeader(Fifo *outputStream, const UdpHeader *udpHeader);

/** Reads a UDP header in network byte order.
 * @param inputStream the bytes received from the network (in network byte order).
 * @param udpHeader little endian byte order UDP header.
 * @return true for successful read, false in case of an error.
 */
bool fifoGetnetUdpHeader(Fifo *inputStream, UdpHeader *udpHeader);

/** Helper function that transforms to network byte order: needed for calculating checksums.
 */
bool fifoPutnetUdpPseudoHeader(Fifo *outputStream, const UdpPseudoHeader *header);

#endif

