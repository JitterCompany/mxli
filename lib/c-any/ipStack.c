#include <ipStack.h>
#include <enetIoFifo.h>

#include <fifo.h>
#include <fifoPrint.h>
#include <fifoParse.h>
#include <string.h>
#include <simpleMath.h>

#include <macros.h>

/* Why IP sucks:
 * + IP-header does not have a fixed size due to the options.
 * + ICMP has to include the variable size IP-header at least in ICMP destination unreachable messages.
 * + TCP and UDP use "pseudo-headers" that access the lower level data of IP => no clean wrapping.
 * + Big Endian order always sucks.
 * + checksum calculation is complicated AND not very effective.
 */

#define DEBUG(x)

DEBUG(
	extern Fifo out;	// debugging output
)

#include <print.h>

void dummyFunction(void) {
	printString("\n\n**** LINK ERROR in ipStack.c\n");
	while (true);
}

// The following avoids linking of any Ethernet driver in the first place
// The correct enetIoXXX-function sets (strong link) are linked when their xxxInit() functions are resolved thereby
// pulling in the hole module
//
bool	enetIoFifoCanRead(void)		__attribute__((weak,alias("dummyFunction")));
Fifo*	enetIoFifoReadBegin(void)	__attribute__((weak,alias("dummyFunction")));
void	enetIoFifoReadEnd(void)		__attribute__((weak,alias("dummyFunction")));

bool	enetIoFifoCanWrite(void)	__attribute__((weak,alias("dummyFunction")));
Fifo*	enetIoFifoWriteBegin(void)	__attribute__((weak,alias("dummyFunction")));
void	enetIoFifoWriteEnd(void)	__attribute__((weak,alias("dummyFunction")));


static const Ip4NetworkConfiguration*	ip4NetworkConfiguration = 0;	///< to be provided by the user
static const Ip4StackHandlers* 		ip4StackHandlers = 0;		///< to be provided by the user
static EthernetArpCache* 		ethernetArpCache = 0;		///< to be provided by the user

void ipStackInit( EthernetArpCache* cache, const Ip4NetworkConfiguration *configuration,
const Ip4StackHandlers *handlers) {

	ip4NetworkConfiguration = configuration;
	ip4StackHandlers = handlers;
	ethernetArpCache = cache;
}

void initIp4Header(Ip4Header *ip4Header) {
	ip4Header->version = 4;
	ip4Header->headerLength4 = sizeof *ip4Header / 4;
	//ip4Header->tos = 0;
	if (ip4Header->totalLength==0) ip4Header->totalLength = sizeof *ip4Header;	// no payload by default
	if (ip4Header->identification==0) ip4Header->identification = 0xdead;		// should be random number
	//ip4Header->flagsAndFragmentOffset8 = 0;
	if (ip4Header->ttl==0) ip4Header->ttl = 255;
	//ip4Header->protocol = 0;	// needs to be set
	ip4Header->headerChecksum = 0;	// good starting point for a calculation
}

static bool handleEthernetIp4IcmpPacket(Fifo *fifo, EthernetHeader const *ethernetHeader, Ip4Header const *ip4Header) {
	IcmpHeader icmpHeader;
	if (fifoGetnetIcmpHeader(fifo,&icmpHeader)) {
		DEBUG( fifoPrintIcmpHeader(&out,&icmpHeader); )
		IcmpEcho icmpEcho;
		if (icmpHeader.type==ICMP_TYPE_ECHO_REQUEST && fifoGetnetIcmpEcho(fifo,&icmpEcho)) {
			DEBUG(
				fifoPrintChar(&out,' ');
				fifoPrintIcmpEcho(&out,&icmpEcho);
			)
			// generate response, if addressed to me
			if (ip4IsEqual(&ip4Header->destinationAddress,&ip4NetworkConfiguration->ip4Address)
			&& enetIoFifoCanWrite()) {
				Fifo *response = enetIoFifoWriteBegin();
				EthernetHeader responseEthernetHeader = {
					.destinationAddress = ethernetHeader->sourceAddress, 
					.sourceAddress = ip4NetworkConfiguration->ethernetAddress,
					.type = ETHERNET_TYPE_IP4,
				};

				Ip4Header responseIp4Header = {
					.sourceAddress = ip4NetworkConfiguration->ip4Address,
					.destinationAddress = ip4Header->sourceAddress,
					.protocol = ip4Header->protocol,	// ICMP in this case
					.totalLength = sizeof(Ip4Header) + sizeof(IcmpHeader) + sizeof(IcmpEcho) + fifoCanRead(fifo),
				};
				responseIp4Header.df = 1;
				initIp4Header(&responseIp4Header);	// add tedious settings

				responseIp4Header.headerChecksum = ~ip4HeaderChecksum(&responseIp4Header);

				IcmpHeader responseIcmpHeader = {
					.type = ICMP_TYPE_ECHO_REPLY,
					.code = 0,
					.checksum = 0,	// to be adjusted shortly...
				};

				responseIcmpHeader.checksum = ~icmpEchoChecksum(&responseIcmpHeader,&icmpEcho,fifo);

				const bool success = 
					fifoPutnetEthernetHeader(response,&responseEthernetHeader)
					&& fifoPutnetIp4Header(response,&responseIp4Header)
					&& fifoPutnetIcmpHeader(response,&responseIcmpHeader)
					&& fifoPutnetIcmpEcho(response,&icmpEcho)
					&& fifoCanRead(fifo)<=fifoCanWrite(response)
					;

				// copy message body back
				while (fifoCanRead(fifo)) fifoWrite(response,fifoRead(fifo));

				// checksum will go here
				enetIoFifoWriteEnd();
				return success;
			}
		}
		DEBUG(fifoPrintLn(&out));
		return true;
	}
	return false;
}

bool handleEthernetIp4TcpPacket(Fifo *fifo, EthernetHeader const *ethernetHeader, Ip4Header const *ip4Header) {
	return false;
}

/** Generates an Ethernet packet suitable for answering a request.
 * @param configuration the local interface's configuration
 * @param request the packet to answer to - address and protocol type are copied from here.
 * @return an Ethernet-header that addresses the sender of the request - use only temporarily
 */
const EthernetHeader* ethernetHeaderReply(Ip4NetworkConfiguration const *configuration, const EthernetHeader *request) {
	static EthernetHeader reply;
	reply.destinationAddress = request->sourceAddress;
	reply.sourceAddress = configuration->ethernetAddress;
	reply.type = request->type;

	return &reply;
}

/** Generates an IP4 packet suitable for answering a request.
 * @param configuration the local interface's configuration
 * @param request the packet to answer to - address and protocol type are copied from here.
 * @param payload the number of embedded bytes.
 * @return an IP4-header that addresses the sender of the request - use only temporarily
 */
const Ip4Header* ip4HeaderReply(Ip4NetworkConfiguration const *configuration, const Ip4Header *request, Uint16 payload) {
	static Ip4Header reply;
	reply.version = 4;
	reply.headerLength4 = sizeof *request / 4;
	reply.tos = request->tos;
	reply.totalLength = sizeof(Ip4Header) + payload;
	reply.identification = 0xfc20;		// should be random number
	reply.flagsAndFragmentOffset8 = 0;
	reply.df = 1;
	reply.ttl = configuration->ttl;
	reply.protocol = request->protocol;
	reply.sourceAddress = configuration->ip4Address;
	reply.destinationAddress = request->sourceAddress;
	reply.headerChecksum = 0;
	reply.headerChecksum = ~ip4HeaderChecksum(&reply);

	return &reply;
}


/** Writes as much of an IP-Header as is possible before knowing the payload
 */
static const Ip4Header* ip4HeaderCreate(Ip4NetworkConfiguration const *configuration, const Ip4Address *destinationAddress,
Uint8 protocol) {
	static Ip4Header reply;
	reply.version = 4;
	reply.headerLength4 = sizeof(Ip4Header) / 4;
	reply.tos = 0;
	reply.totalLength = sizeof(Ip4Header) + 0;	// payload unknown
	reply.identification = 0xfc20;		// should be random number
	reply.flagsAndFragmentOffset8 = 0;
	reply.df = 1;
	reply.ttl = configuration->ttl;
	reply.protocol = protocol;
	reply.sourceAddress = configuration->ip4Address;
	reply.destinationAddress = *destinationAddress;
	reply.headerChecksum = 0;

	return &reply;
}

//UNUSED:
/** Rewrites the IP header with correct length and checksum.
 * @param rewriter a fifo set up for rewriting
 * @param reader a fifo containing the whole IP packet.
 */
void ip4HeaderRewrite(Fifo *rewriter, Fifo *reader) {
	Ip4Header ip4Header;
	fifoGetnetIp4Header(reader,&ip4Header);
	ip4Header.totalLength = fifoCanRead(reader);
	ip4Header.headerChecksum = ~ip4HeaderChecksum(&ip4Header);
	fifoPutnetIp4Header(rewriter,&ip4Header);
}

/** Rewrites both IP and UDP header with correct length and checksum.
 * @param rewriter a fifo set up for rewriting
 * @param reader a fifo containing the whole IP packet.
 */
void ip4IpUdpHeaderRewrite(Fifo *rewriter, Fifo *reader) {
	// IP header
	Ip4Header ip4Header;
	fifoGetnetIp4Header(reader,&ip4Header);
	ip4Header.totalLength = sizeof(Ip4Header) + fifoCanRead(reader);
	ip4Header.headerChecksum = ~ip4HeaderChecksum(&ip4Header);
	fifoPutnetIp4Header(rewriter,&ip4Header);

	// UDP header
	UdpHeader udpHeader;
	fifoGetnetUdpHeader(reader,&udpHeader);
	udpHeader.checksum = 0;
	udpHeader.length = sizeof(UdpHeader) + fifoCanRead(reader);
	UdpPseudoHeader udpPseudoHeader = {
		.sourceAddress = ip4Header.sourceAddress,
		.destinationAddress = ip4Header.destinationAddress,
		.mbz = 0,
		.protocol = ip4Header.protocol,
		.udpLength = sizeof(UdpHeader) + fifoCanRead(reader),	// header and data
	};
	udpHeader.checksum = ~onesComplementAdd16(
		udpPseudoHeaderChecksum(&udpPseudoHeader),
		onesComplementAdd16(udpHeaderChecksum(&udpHeader), fifoCalculateOnesComplementSumBigEndian(reader))
	);
	fifoPutnetUdpHeader(rewriter,&udpHeader);
}

//UNUSED:
/** Rewrites the UDP header with correct length and disables checksum.
 * @param rewriter a fifo set up for rewriting
 * @param reader a fifo containing the whole IP packet.
 */
void ip4UdpHeaderRewrite(Fifo *rewriter, Fifo *reader) {
	UdpHeader udpHeader;
	fifoGetnetUdpHeader(reader,&udpHeader);
	udpHeader.length = sizeof(UdpHeader) + fifoCanRead(reader);
	udpHeader.checksum = 0;	// disable.
	fifoPutnetUdpHeader(rewriter,&udpHeader);
}

/** Find the Ethernet address of the destination of an IP packet.
 * If it is a broadcast address, then use broadcast.
 * If it is in the current subnet, then it should be in the ARP cache
 * Otherwise send it to the gateway (which in turn should be in the ARP cache).
 */
const EthernetAddress* destinationEthernetAddress(const Ip4Address *destination) {
	const EthernetArpCacheEntry *cacheEntry = 0;

	if (ip4IsBroadcast(destination)) return &ETHERNET_ADDRESS_BROADCAST;
	if (ip4IsSubnet(&ip4NetworkConfiguration->ip4Address,ip4NetworkConfiguration->subnetBits,destination))
		cacheEntry = ethernetArpCacheLookup(ethernetArpCache,destination,sysTickTimeMs);
	else cacheEntry = ethernetArpCacheLookup(ethernetArpCache,&ip4NetworkConfiguration->gateway,sysTickTimeMs);
	if (cacheEntry!=0) return &cacheEntry->ethernetAddress;
	else return 0;
}

Fifo* udpSendBegin(const Ip4SocketAddress *destination, Uint16 sourcePort) {
	if (!enetIoFifoCanWrite()) return 0;

	Fifo *outputStream = enetIoFifoWriteBegin();
	// look up hardware address from ARP cache.
	EthernetAddress const* ethernetAddress = destinationEthernetAddress(&destination->ip4Address);
	if (ethernetAddress==0) {
		DEBUG(
			fifoPrintString(&out,"ERROR: no ARP cache entry for:");
			fifoPrintIp4Address(&out,&destination->ip4Address);
			fifoPrintLn(&out);
		)
		return 0;
	}

	// write Ethernet header
	EthernetHeader ethernetHeader = {
		.destinationAddress = *ethernetAddress,
		.sourceAddress = ip4NetworkConfiguration->ethernetAddress,
		.type = ETHERNET_TYPE_IP4
	};
	fifoPutnetEthernetHeader(outputStream,&ethernetHeader);

	// write IP4 header
	fifoPutnetIp4Header(outputStream,
		ip4HeaderCreate(ip4NetworkConfiguration,&destination->ip4Address,IP_PROTOCOL_UDP));

	// write UDP header
	UdpHeader responseUdpHeader = {
		.source = sourcePort,
		.destination = destination->port,
		.length = sizeof(UdpHeader) + 0,
		.checksum = 0,	// unused if 0
	};
	fifoPutnetUdpHeader(outputStream,&responseUdpHeader);
	// now comes the UDP contents.
	return outputStream;
}

void udpSendEnd(Fifo *fifo) {
	Fifo reader = *fifo;

	fifoSkipRead(&reader,sizeof(EthernetHeader));	// Ethernet header is already fine.

	// rewrite IP,UDP
	Fifo rewriter;
	fifoInitRewrite(&rewriter,&reader);
	
	ip4IpUdpHeaderRewrite(&rewriter,&reader);	// checksums, lengths...
	enetIoFifoWriteEnd();
}

/** Sends back an ICMP destination unreachable message.
 */
bool icmpDestinationUnreachable(EthernetHeader const *ethernetHeader, Ip4Header const *ip4Header, int code,
Fifo *ip4Datagram) {
	// ICMP port unreachable / fragmented
	if (enetIoFifoCanWrite()) {
		Fifo *replyFifo = enetIoFifoWriteBegin();

		// reply must include original IP-header + 64bits of packet body.
		const int ip4ReplyLength = ip4Header->headerLength4*4 + 64/8;

		// Ethernet
		EthernetHeader replyEthernetHeader = {
			.destinationAddress = ethernetHeader->sourceAddress,
			.sourceAddress = ip4NetworkConfiguration->ethernetAddress,
			.type = ETHERNET_TYPE_IP4
		};
		fifoPutnetEthernetHeader(replyFifo,&replyEthernetHeader);

		// IP4
		Ip4Header replyIp4Header = {
			.totalLength = sizeof(Ip4Header) + sizeof(IcmpHeader) + 4 + ip4ReplyLength,
			.protocol = IP_PROTOCOL_ICMP,
			.identification = ip4Header->identification,		// needed to identify packet.
			.sourceAddress = ip4NetworkConfiguration->ip4Address,
			.destinationAddress = ip4Header->sourceAddress,
		};
		replyIp4Header.df = 1;
		initIp4Header(&replyIp4Header);
		replyIp4Header.headerChecksum = ~ip4HeaderChecksum(&replyIp4Header);
		fifoPutnetIp4Header(replyFifo, &replyIp4Header);

		Fifo rewriter = *replyFifo;	// needed to adjust ICMP checksum later.
		// ICMP
		IcmpHeader replyIcmpHeader = {
			.type = ICMP_TYPE_DESTINATION_UNREACHABLE,
			.code = code,
			.checksum = 0
		};
		fifoPutnetIcmpHeader(replyFifo,&replyIcmpHeader);
		fifoPutnetInt32(replyFifo,0);	// 'unused' field of destination unreachable message

		if (fifoCanRead(ip4Datagram)>=ip4ReplyLength
		&& fifoCanWrite(replyFifo)>=ip4ReplyLength) for (int o=0; o<ip4ReplyLength; ++o) {
			fifoWrite(replyFifo, fifoRead(ip4Datagram));
		}
		else return PACKET_DISCARDED;

		// adjust checksum
		Fifo icmpReader = *replyFifo;
		fifoSkipToRead(&icmpReader,rewriter.wPos);	// read from ICMP header on, only
		replyIcmpHeader.checksum = ~fifoCalculateOnesComplementSumBigEndian(&icmpReader);
		fifoPutnetIcmpHeader(&rewriter,&replyIcmpHeader);

		enetIoFifoWriteEnd();
		return PACKET_HANDLED;
	}
	else return PACKET_RETRY;	// try later
}

InputPacketStatus handleEthernetIp4UdpPacket(Fifo *inputStream, EthernetHeader const *ethernetHeader, Ip4Header const *ip4Header) {
	UdpHeader udpHeader;
	if (fifoGetnetUdpHeader(inputStream,&udpHeader)) {
		DEBUG(fifoPrintUdpHeader(&out,&udpHeader));

		// truncate inputStream to the size of the UDP packet.
		const unsigned length = MIN(udpHeader.length, fifoCanRead(inputStream));
		Fifo payloadFifo;
		if (!fifoParseN(inputStream,&payloadFifo,length)) {
			DEBUG(fifoPrintString(&out,"UDP invalid length packet.\n"));
			return PACKET_DISCARDED;	// discard it
		}

		if (ip4StackHandlers!=0 && ip4StackHandlers->udp!=0) {
			Ip4SocketAddress udpAddress = {
				.ip4Address = ip4Header->sourceAddress,
				.port = udpHeader.source,
			};
			return ip4StackHandlers->udp(&payloadFifo,udpHeader.destination,&udpAddress);
		}
		else return PACKET_PENDING;
	}
	else {
		DEBUG(fifoPrintString(&out,"INVALID UDP packet.\n"));
		return PACKET_DISCARDED;
	}
}

InputPacketStatus handleEthernetIp4Packet(Fifo *fifo, const EthernetHeader *ethernetHeader) {
	Ip4Header ip4Header;
	Fifo ip4Reader = *fifo;	// backup for ICMP port unreachable
	if (fifoGetnetIp4Header(fifo,&ip4Header)) {
		DEBUG( fifoPrintIp4Header(&out,&ip4Header); )

		// update ARP cache from received packets
		if (ip4IsSubnet(&ip4NetworkConfiguration->ip4Address,
			ip4NetworkConfiguration->subnetBits,&ip4Header.sourceAddress)) {
			ethernetArpCacheSet(
				ethernetArpCache,
				&ip4Header.sourceAddress,
				&ethernetHeader->sourceAddress,
				sysTickTimeMs
			);
			DEBUG(
				fifoPrintString(&out,"[ARP: ");
				fifoPrintIp4Address(&out,&ip4Header.sourceAddress);
				fifoPrintString(&out,"-->");
				fifoPrintEthernetAddress(&out,&ethernetHeader->sourceAddress);
				fifoPrintChar(&out,']');
			)
		}
		InputPacketStatus status;
		switch(ip4Header.protocol) {
		case IP_PROTOCOL_ICMP:	return handleEthernetIp4IcmpPacket(fifo,ethernetHeader,&ip4Header);
		case IP_PROTOCOL_IGMP:	return PACKET_PENDING;
		case IP_PROTOCOL_TCP:	return handleEthernetIp4TcpPacket(fifo,ethernetHeader,&ip4Header);
		case IP_PROTOCOL_UDP:	status = handleEthernetIp4UdpPacket(fifo,ethernetHeader,&ip4Header);
					if (status!=PACKET_PENDING) return status;
					else return icmpDestinationUnreachable(
						ethernetHeader,&ip4Header,
						ICMP_CODE_PORT_UNREACHABLE,&ip4Reader);
		default:		DEBUG( fifoPrintString(&out,"Unknown IP4 subprotocol.\n") );
					return PACKET_PENDING;	
		}
	}
	else return false;	
}

InputPacketStatus ethernetArpRequest(const Ip4Address* ip4Address) {
	if (enetIoFifoCanWrite()) {
		Fifo *writer = enetIoFifoWriteBegin();
		const EthernetHeader ethernetHeader = {
			.destinationAddress = ETHERNET_ADDRESS_BROADCAST,
			.sourceAddress = ip4NetworkConfiguration->ethernetAddress,
			.type = ETHERNET_TYPE_ARP,
		};
		const EthernetArpHeader ethernetArpHeader = {
			.hardwareAddressType = 1,
			.protocol = ETHERNET_TYPE_IP4,
			.hardwareAddressSize = sizeof(EthernetAddress),
			.protocolAddressSize = sizeof(Ip4Address),
			.operation = 1,		// request
			.sourceEthernetAddress = ip4NetworkConfiguration->ethernetAddress,
			.destinationIp4Address = *ip4Address,
			.sourceIp4Address = ip4NetworkConfiguration->ip4Address,
			.destinationEthernetAddress = ETHERNET_ADDRESS_INVALID,
		};
		fifoPutnetEthernetHeader(writer,&ethernetHeader);
		fifoPutnetEthernetArpHeader(writer,&ethernetArpHeader);
		// no message body
		enetIoFifoWriteEnd();
		return PACKET_HANDLED;
	}
	else return PACKET_RETRY;
}

InputPacketStatus handleEthernetArpPacket(Fifo *fifo, const EthernetHeader *ethernetHeader) {
	EthernetArpHeader arpHeader;
	if (fifoGetnetEthernetArpHeader(fifo,&arpHeader)) {
		DEBUG( fifoPrintEthernetArpHeader(&out,&arpHeader); )
		// remember IP address of sender
		ethernetArpCacheSet(
			ethernetArpCache,
			&arpHeader.sourceIp4Address,
			&arpHeader.sourceEthernetAddress,
			sysTickTimeMs
		);
		DEBUG( fifoPrintLn(&out); )
		if (ip4IsEqual(&arpHeader.destinationIp4Address,&ip4NetworkConfiguration->ip4Address)
		&& arpHeader.operation==1	// request
		&& enetIoFifoCanWrite()) {
			Fifo *response = enetIoFifoWriteBegin();
			const EthernetHeader responseEthernetHeader = {
				.destinationAddress = ethernetHeader->sourceAddress,
				.sourceAddress = ip4NetworkConfiguration->ethernetAddress,
				.type = ETHERNET_TYPE_ARP,
			};
			arpHeader.operation = 2;	// response type
			arpHeader.sourceEthernetAddress = ip4NetworkConfiguration->ethernetAddress;
			arpHeader.destinationIp4Address = arpHeader.sourceIp4Address;
			arpHeader.sourceIp4Address = ip4NetworkConfiguration->ip4Address;
			arpHeader.destinationEthernetAddress = ethernetHeader->sourceAddress;

			fifoPutnetEthernetHeader(response,&responseEthernetHeader);
			fifoPutnetEthernetArpHeader(response,&arpHeader);
			fifoPrintString(response,"FC200 rules!");
			enetIoFifoWriteEnd();
			DEBUG(
				fifoPrintString(&out,"Sent back: ");
				fifoPrintEthernetArpHeader(&out,&arpHeader);
				fifoPrintLn(&out);
			)
		}
		return PACKET_HANDLED;
	}
	else {
		DEBUG( fifoPrintString(&out,"invalid ARP packet - IGNORED.\n"); )
		return PACKET_DISCARDED;
	}
}

InputPacketStatus handleEthernetPacket(Fifo *fifo) {
	DEBUG(
		fifoPrintString(&out,"\nFrame received, raw size=");
		fifoPrintUDec(&out,fifoCanRead(fifo),1,4);
		fifoPrintString(&out,"B\n");
	)
	EthernetHeader ethernetHeader;
	if (fifoGetnetEthernetHeader(fifo,&ethernetHeader)) {
		DEBUG(
			fifoPrintString(&out,"Ethernet dst=");
			fifoPrintEthernetAddress(&out,&ethernetHeader.destinationAddress);
			fifoPrintString(&out,", src=");
			fifoPrintEthernetAddress(&out,&ethernetHeader.sourceAddress);
			fifoPrintString(&out,", type=0x");
			fifoPrintHex(&out,ethernetHeader.type,4,4);
			fifoPrintLn(&out);
		)
		switch(ethernetHeader.type) {
		case ETHERNET_TYPE_ARP: return handleEthernetArpPacket(fifo,&ethernetHeader);
		case ETHERNET_TYPE_IP4: return handleEthernetIp4Packet(fifo,&ethernetHeader);
		default:		DEBUG( fifoPrintString(&out,"unknown protocol 0x");
						fifoPrintHex(&out,ethernetHeader.type,4,4);
						fifoPrintLn(&out)
					);
					return PACKET_PENDING;
		}
	}
	else {
		DEBUG( fifoPrintString(&out,"Frame corrupted - IGNORED.\n"); )
		return PACKET_DISCARDED;
	}
}

