#ifndef __ethernet_h
#define __ethernet_h

/** @file
 * @brief Basic definition of Ethernet packets.
 *
 */

#include <stdbool.h>
#include <integers.h>
#include <fifo.h>

/** Ethernet MAC address in network byte order.
 */
typedef union {
	Uint8		int8s[6];
	Uint16		int16s[3];
} EthernetAddress;

/** Converts an ethernet MAC into a human-readable string.
 * @param fifo the string destination.
 * @param ethernetAddress the Ethernet MAC.
 * @return true in case of success, false if formatBuffer is exhausted.
 */
bool fifoPrintEthernetAddress(Fifo *fifo, const EthernetAddress* ethernetAddress);

/** Parses an Ethernet MAC from a String.
 * @param fifo a Fifo containing the string. A MAC address looks like this:
 *   12:34:56:78:90:AB .
 * @param ethernetAddress the result value (an array - always passed as pointer in C).
 * @return true if successful, false otherwise (and iterator is restored).
 */
bool fifoParseEthernetAddress(Fifo *fifo, EthernetAddress* ethernetAddress);

/** Reads an Ethernet Address from a binary stream in network byte order.
 * @param fifo a Fifo containing the address. 
 * @param ethernetAddress the result value (an array - always passed as pointer in C).
 * @return true if successful, false otherwise (and iterator is restored).
 */
static inline bool fifoGetnetEthernetAddress(Fifo *fifo, EthernetAddress* ethernetAddress) {
	return fifoGetN(fifo,ethernetAddress,sizeof *ethernetAddress);
}

/** Writes an Ethernet Address into a binary stream in network byte order.
 */
static inline bool fifoPutnetEthernetAddress(Fifo *fifo, const EthernetAddress *ethernetAddress) {
	return fifoPutN(fifo,ethernetAddress,sizeof *ethernetAddress);
}

/** Compares two Ethernet addresses.
 * @return true, if the two addresses are equal.
 */
bool ethernetAddressIsEqual(const EthernetAddress *a1, const EthernetAddress *a2);

/** Ethernet type II packet (tagged ethernet frame), not in network byte order.
 */
typedef struct __attribute__((packed, aligned(2))) {
	EthernetAddress		destinationAddress;
	EthernetAddress		sourceAddress;
	Uint16			type;	
} EthernetHeader;

bool fifoPrintEthernetHeader(Fifo *fifo, EthernetHeader const *ethernetHeader);
bool fifoGetnetEthernetHeader(Fifo *fifo, EthernetHeader *ethernetHeader);
bool fifoPutnetEthernetHeader(Fifo *fifo, EthernetHeader const *ethernetHeader);

union EthernetVlanHeader {
	struct __attribute__((packed)) {
		EthernetAddress		destinationAddress;
		EthernetAddress		sourceAddress;
		union {
			struct __attribute__((packed)) {
				Uint16	type;	
				Uint8	data[];
			};
			struct __attribute__((packed)) {
				Uint16	typeVlan;	// must be changeEndian16(0x8100);
				Uint16	tci;
				Uint16	type;		// packet type
				Uint8	data[];
			} vlanTag;
		};
	};
	Uint8 	int8s[18];	// size of VLAN frame without CRC
	Uint16	int16s[9];
};
typedef union EthernetVlanHeader EthernetVlanHeader;

enum {
	ETHERNET_TYPE_IP4	=0x0800,	///< network byte order
	ETHERNET_TYPE_ARP	=0x0806,	///< network byte order
};

extern const EthernetAddress ETHERNET_ADDRESS_BROADCAST;
extern const EthernetAddress ETHERNET_ADDRESS_INVALID;

#endif
