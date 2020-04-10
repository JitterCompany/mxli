#ifndef __ip4Tcp_h
#define __ip4Tcp_h

/** @file
 * @brief IP4/TCP low-level IO.
 */

#include <ip4.h>

/** TCP header format, in local processor bit/byte order.
 */
typedef struct __attribute__((packed,aligned(2))) {
	Uint16		source;			///< source port number
	Uint16		destination;		///< destination port number
	/** Sending sequence number. If SYN=1, then first octet is sequence+1 else sequence.
	 */
	Uint32		sequence;
	Uint32		acknowledge;		///< acknowledge sequence number (next expected sequence number).
	union {
		struct __attribute__((packed,aligned(4))) {
		/*
			Uint16	dataOffset4:4;		///< The number of 32bit words of the TCP header. 
			Uint16	reserved:6;		///< Must be zero
			Uint16	urg:1;			///< Urgent pointer field significant
			Uint16	ack:1;			///< Acknowledge field significant
			Uint16	psh:1;			///< Push function
			Uint16	rst:1;			///< Reset connection
			Uint16	syn:1;			///< Synchronized sequence numbers
			Uint16	fin:1;			///< No more data from sender
		*/
			// crazy ordering of bits.
			Uint16	fin:1;			///< No more data from sender
			Uint16	syn:1;			///< Synchronized sequence numbers
			Uint16	rst:1;			///< Reset connection
			Uint16	psh:1;			///< Push function
			Uint16	ack:1;			///< Acknowledge field significant
			Uint16	urg:1;			///< Urgent pointer field significant
			Uint16	reserved:6;		///< Must be zero
			Uint16	dataOffset4:4;		///< The number of 32bit words of the TCP header. 
		};
		Uint16	offsetAndFlags;			///< for easy reversal of endianess
	};
	Uint16		window;			///< The number of octets we can receive, beginnning from acknowledge
	/** 16bit one's complement of the 1's complement sum of all 16bit words in the pseudo-header, header and data.
	 * Right padded with 0 if odd number of data bits. 
	 */
	Uint16		checksum;
	Uint16		urgentPointer;		///< if urg=1 this indicates start of urgent data from sequence.
	Uint8		options[];		///< variable number, zero-padded to 32-bit boundary.	
} TcpHeader;

typedef struct __attribute__((packed,aligned(2))) {
	Ip4Address	source;
	Ip4Address	destination;
	Uint8		zero;
	Uint8		protocol;
	Uint16		tcpLength;	///< TCP header + data, without pseudo-header.
} TcpPseudoHeader;

/** Options in network byte order (uninterpreted).
 */
typedef struct __attribute__((packed,aligned(2))) {
	Uint8		uint8s[40];
} TcpOptions;

enum {
	TCP_OPTION_EOL	=0,		///< end of option list (before 32-bit bound)
	TCP_OPTION_NOP	=1,		///< extremely important...
	TCP_OPTION_MSS	=2,		///< maximum seqment size. Next byte=4 (size), followed by 16-bit MSS.
};

/** Erases all TcpOptions.
 */
void tcpOptionsInit(TcpOptions *tcpOptions);

/** Writes the MSS option parameter.
 */
bool tcpOptionsPutnetMss(TcpOptions *tcpOptions, int mss);

/** Calculates the size of the options part of a TCP header.
 * @return the size as a multiple of 32bits.
 */
size_t tcpOptionsSize(TcpOptions const *tcpOptions);

/** Calculates the TCP header checksum, that follows the pseudo-header (conceptionally).
 * This is 1 out of 3 parts of the TCP checksum.
 */
Uint16 tcpHeaderChecksum(TcpHeader const* header);

/** Calculates the TCP pseudo-header checksum used in the TCP header.
 * This is 1 out of 3 parts of the TCP checksum.
 */
Uint16 tcpPseudoHeaderChecksum(TcpPseudoHeader const* header);

/** prints TCP options in human-readable form.
 */
bool fifoPrintTcpOptions(Fifo *console, const TcpOptions *options);

/** Prints an TCP-header in human-readable form.
 */
bool fifoPrintTcpHeader(Fifo *console, const TcpHeader *udpHeader, const TcpOptions *options);
bool fifoPutnetTcpHeader(Fifo *outputStream, const TcpHeader *udpHeader, const TcpOptions *options);
bool fifoGetnetTcpHeader(Fifo *inputStream, TcpHeader *udpHeader, TcpOptions *options);

bool fifoPutnetTcpPseudoHeader(Fifo *outputStream, const TcpPseudoHeader *header);

#endif
