//HEADER
#include <ip4.h>
#include <fifoParse.h>
#include <fifoPrint.h>
#include <simpleMath.h>

//SLICE
Ip4Address IP4_ADDRESS_BROADCAST = { .int8s = {255,255,255,255} };

//SLICE
bool fifoPrintIp4Address(Fifo *fifo, const Ip4Address* ip4Address) {
	return
		fifoPrintUDec(fifo,ip4Address->int8s[0],1,3)
		&& fifoPrintChar(fifo,'.')
		&& fifoPrintUDec(fifo,ip4Address->int8s[1],1,3)
		&& fifoPrintChar(fifo,'.')
		&& fifoPrintUDec(fifo,ip4Address->int8s[2],1,3)
		&& fifoPrintChar(fifo,'.')
		&& fifoPrintUDec(fifo,ip4Address->int8s[3],1,3)
		;
}

//SLICE
bool fifoParseIp4Address(Fifo *fifo, Ip4Address* ip4Address) {
	Fifo clone = *fifo;

	unsigned parsedAddress[4];
	if (fifoParseUnsignedLimited(&clone,&parsedAddress[0],0,255)
	&& fifoParseExactChar(&clone,'.')
	&& fifoParseUnsignedLimited(&clone,&parsedAddress[1],0,255)
	&& fifoParseExactChar(&clone,'.')
	&& fifoParseUnsignedLimited(&clone,&parsedAddress[2],0,255)
	&& fifoParseExactChar(&clone,'.')
	&& fifoParseUnsignedLimited(&clone,&parsedAddress[3],0,255) ) {
		for (int i=0; i<4; ++i) ip4Address->int8s[i] = parsedAddress[i];
		fifoCopyReadPosition(fifo,&clone);
		return true;
	}
	else return false;
}

//SLICE
bool fifoPrintIp4Header(Fifo *fifo, const Ip4Header* ip4Header) {
	bool ok = fifoPrintString(fifo,"IPv")
		&& fifoPrintUDec(fifo,ip4Header->version,1,3)
		&& fifoPrintString(fifo," HL=")
		&& fifoPrintUDec(fifo,4*ip4Header->headerLength4,1,3)
		&& fifoPrintString(fifo," PROT=")
		;

	switch(ip4Header->protocol) {
		case IP_PROTOCOL_ICMP:	ok = ok && fifoPrintString(fifo,"ICMP"); break;
		case IP_PROTOCOL_IGMP:	ok = ok && fifoPrintString(fifo,"IGMP"); break;
		case IP_PROTOCOL_TCP:	ok = ok && fifoPrintString(fifo,"TCP"); break;
		case IP_PROTOCOL_UDP:	ok = ok && fifoPrintString(fifo,"UDP"); break;
		default:		ok = ok && fifoPrintString(fifo,"prot=0x")
						&& fifoPrintHex(fifo,ip4Header->protocol,2,2);
	}
	return ok
		&& fifoPrintString(fifo," TOS=")
		&& fifoPrintUDec(fifo,ip4Header->tos,1,5)
		&& fifoPrintString(fifo," TL=")
		&& fifoPrintUDec(fifo,ip4Header->totalLength,1,5)
		&& fifoPrintString(fifo," DF=")
		&& fifoPrintUDec(fifo,ip4Header->df,1,1)
		&& fifoPrintString(fifo," MF=")
		&& fifoPrintUDec(fifo,ip4Header->mf,1,1)
		&& fifoPrintString(fifo," fragment offset=")
		&& fifoPrintUDec(fifo, 8*ip4Header->fragmentOffset8,1,5)
		&& fifoPrintString(fifo," src=")
		&& fifoPrintIp4Address(fifo,&ip4Header->sourceAddress)
		&& fifoPrintString(fifo," dst=")
		&& fifoPrintIp4Address(fifo,&ip4Header->destinationAddress)
		&& fifoPrintChar(fifo,'}');
}

//SLICE
bool fifoGetnetIp4Header(Fifo *fifo, Ip4Header *ip4Header) {
	Fifo clone = *fifo;
	// everything up to the options
	if (sizeof *ip4Header <= fifoCanRead(fifo)) {
		fifoGetUint8(&clone,&ip4Header->versionAndHeaderLength4);
		fifoGetUint8(&clone,&ip4Header->tos);
		fifoGetnetUint16(&clone,&ip4Header->totalLength);
		fifoGetnetUint16(&clone,&ip4Header->identification);
		fifoGetnetUint16(&clone,&ip4Header->flagsAndFragmentOffset8);
		fifoGetUint8(&clone,&ip4Header->ttl);
		fifoGetUint8(&clone,&ip4Header->protocol);
		fifoGetnetUint16(&clone,&ip4Header->headerChecksum);
		fifoGetnetIp4Address(&clone,&ip4Header->sourceAddress);
		fifoGetnetIp4Address(&clone,&ip4Header->destinationAddress);

		// options may follow: up to 4*15 - 20 = 40bytes
		// remove options
		const unsigned optionsSize = ip4Header->headerLength4*4 - sizeof *ip4Header;
		if (optionsSize<=fifoCanRead(&clone)) {
			fifoSkipRead(&clone,optionsSize);
			fifoCopyReadPosition(fifo,&clone);
			return true;
		}
		else return false;	// options underrun.
	}
	else return false;
}

//SLICE
/** No options used.
 */
bool fifoPutnetIp4Header(Fifo *fifo, Ip4Header const *ip4Header) {
	if (sizeof *ip4Header <= fifoCanWrite(fifo)) {
		fifoPutInt8(fifo,ip4Header->versionAndHeaderLength4);
		fifoPutInt8(fifo,ip4Header->tos);
		fifoPutnetInt16(fifo,ip4Header->totalLength);
		fifoPutnetInt16(fifo,ip4Header->identification);
		fifoPutnetInt16(fifo,ip4Header->flagsAndFragmentOffset8);
		fifoPutInt8(fifo,ip4Header->ttl);
		fifoPutInt8(fifo,ip4Header->protocol);
		fifoPutnetInt16(fifo,ip4Header->headerChecksum);
		fifoPutnetIp4Address(fifo,&ip4Header->sourceAddress);
		fifoPutnetIp4Address(fifo,&ip4Header->destinationAddress);
		return true;
	}
	else return false;
}

//SLICE
/** Calculates the one's complement checksum of the fifo contents viewed as BigEndian 16bit values.
 * The fifo is left unmodified.
 */
Uint16 fifoCalculateOnesComplementSumBigEndian(Fifo *fifo) {
	Fifo clone = *fifo;
	Uint16 sum = 0;
	while (2<=fifoCanRead(&clone)) {
		const int word = (Uint8)fifoRead(&clone) | (Uint8)fifoRead(&clone) << 8;
		sum = onesComplementAdd16(sum,changeEndian16(word));
	}
	// additional last byte, not on a 16-bit boundary
	if (1==fifoCanRead(&clone)) sum = onesComplementAdd16(sum, changeEndian16((Uint8)fifoRead(&clone)));
	return sum;
}

//SLICE
/** Header without options.
 */
Uint16 ip4HeaderChecksum(Ip4Header const *header) {
	char buffer[sizeof *header];
	Fifo fifo = { buffer, sizeof buffer };
	fifoPutnetIp4Header(&fifo,header);
	return fifoCalculateOnesComplementSumBigEndian(&fifo);
}

//SLICE
bool fifoPrintIp4SocketAddress(Fifo *fifo, const Ip4SocketAddress* socketAddress) {
	return	fifoPrintIp4Address(fifo,&socketAddress->ip4Address)
		&& fifoPrintChar(fifo,':')
		&& fifoPrintUDec(fifo,socketAddress->port,1,5);
}

