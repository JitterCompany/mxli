//HEADER
#include <ip4Icmp.h>
#include <fifoParse.h>
#include <fifoPrint.h>
#include <simpleMath.h>

//SLICE
bool fifoGetnetIcmpHeader(Fifo *fifo, IcmpHeader *icmpHeader) {
	if (sizeof *icmpHeader <= fifoCanRead(fifo)) {
		fifoGetUint8(fifo,&icmpHeader->type);
		fifoGetUint8(fifo,&icmpHeader->code);
		fifoGetnetUint16(fifo,&icmpHeader->checksum);
		return true;
	}
	else return false;
}

//SLICE
bool fifoPutnetIcmpHeader(Fifo *fifo, IcmpHeader const *icmpHeader) {
	if (sizeof *icmpHeader <= fifoCanWrite(fifo)) {
		fifoPutInt8(fifo,icmpHeader->type);
		fifoPutInt8(fifo,icmpHeader->code);
		fifoPutnetInt16(fifo,icmpHeader->checksum);
		return true;
	}
	else return false;
}

//SLICE
/** Human readable console output.
 */
bool fifoPrintIcmpHeader(Fifo *fifo, IcmpHeader const *header) {
	bool success = fifoPrintString(fifo,"ICMP type=");
	switch(header->type) {
	case ICMP_TYPE_ECHO_REPLY:	success = success && fifoPrintString(fifo,"ECHO REPLY"); break;
	case ICMP_TYPE_ECHO_REQUEST:	success = success && fifoPrintString(fifo,"ECHO REQUEST"); break;
	default: success = success && fifoPrintUDec(fifo,header->type,1,3);
	}

	return	success
		&& fifoPrintString(fifo," code=")
		&& fifoPrintUDec(fifo,header->code,1,3)	
		&& fifoPrintString(fifo," cksum=0x")
		&& fifoPrintHex(fifo,header->checksum,4,4);
}

//SLICE
bool fifoGetnetIcmpEcho(Fifo *fifo, IcmpEcho *icmpEcho) {
	if (sizeof *icmpEcho <= fifoCanRead(fifo)) {
		fifoGetnetUint16(fifo,&icmpEcho->identifier);
		fifoGetnetUint16(fifo,&icmpEcho->sequenceNumber);
		return true;
	}
	else return false;
}

//SLICE
bool fifoPutnetIcmpEcho(Fifo *fifo, IcmpEcho const *icmpEcho) {
	if (sizeof *icmpEcho <= fifoCanWrite(fifo)) {
		fifoPutnetInt16(fifo,icmpEcho->identifier);
		fifoPutnetInt16(fifo,icmpEcho->sequenceNumber);
		return true;
	}
	else return false;
}

//SLICE
bool fifoPrintIcmpEcho(Fifo *fifo, IcmpEcho const *icmpEcho) {
	return	fifoPrintString(fifo,"id=0x")
		&& fifoPrintHex(fifo,icmpEcho->identifier,4,4)
		&& fifoPrintString(fifo," seq=")
		&& fifoPrintUDec(fifo,icmpEcho->sequenceNumber,1,5);
}

//SLICE
Uint16 icmpHeaderChecksum(IcmpHeader const *icmpHeader) {
	char buffer[sizeof *icmpHeader];
	Fifo fifo = { buffer, sizeof buffer };
	fifoPutnetIcmpHeader(&fifo,icmpHeader);
	return fifoCalculateOnesComplementSumBigEndian(&fifo);
}

//SLICE
Uint16 icmpEchoChecksum(IcmpHeader const *icmpHeader, IcmpEcho const *icmpEcho, Fifo *data) {
	char buffer[sizeof *icmpHeader + sizeof *icmpEcho];
	Fifo fifo = { buffer, sizeof buffer };
	fifoPutnetIcmpHeader(&fifo,icmpHeader);
	fifoPutnetIcmpEcho(&fifo,icmpEcho);
	return onesComplementAdd16(
		fifoCalculateOnesComplementSumBigEndian(&fifo),
		fifoCalculateOnesComplementSumBigEndian(data)
	);
}


