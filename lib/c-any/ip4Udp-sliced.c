//HEADER

#include <ip4Udp.h>
#include <fifoParse.h>
#include <fifoPrint.h>
#include <simpleMath.h>

//SLICE
bool fifoPutnetUdpHeader(Fifo *outputStream, const UdpHeader *udp) {
	if (sizeof *udp <= fifoCanWrite(outputStream)) {
		fifoPutnetInt16(outputStream,udp->source);
		fifoPutnetInt16(outputStream,udp->destination);
		fifoPutnetInt16(outputStream,udp->length);
		fifoPutnetInt16(outputStream,udp->checksum);
		return true;
	}
	else return false;
}

//SLICE
bool fifoGetnetUdpHeader(Fifo *inputStream, UdpHeader *udp) {
	if (sizeof *udp <= fifoCanRead(inputStream)) {
		fifoGetnetUint16(inputStream,&udp->source);
		fifoGetnetUint16(inputStream,&udp->destination);
		fifoGetnetUint16(inputStream,&udp->length);
		fifoGetnetUint16(inputStream,&udp->checksum);
		return true;
	}
	else return false;
}

//SLICE
bool fifoPutnetUdpPseudoHeader(Fifo *outputStream, const UdpPseudoHeader *header) {
	if (sizeof *header <= fifoCanWrite(outputStream)) {
		fifoPutnetIp4Address(outputStream, &header->sourceAddress);
		fifoPutnetIp4Address(outputStream, &header->destinationAddress);
		fifoPutInt8(outputStream, header->mbz);
		fifoPutInt8(outputStream, header->protocol);
		fifoPutnetInt16(outputStream, header->udpLength);
		return true;
	}
	else return false;
}

//SLICE
Uint16 udpPseudoHeaderChecksum(UdpPseudoHeader const *pseudoHeader) {
	UdpPseudoHeader uph;
	Fifo fifo = { (void*)&uph, sizeof uph };
	fifoPutnetUdpPseudoHeader(&fifo,pseudoHeader);
	return fifoCalculateOnesComplementSumBigEndian(&fifo);
}

//SLICE
Uint16 udpHeaderChecksum(UdpHeader const* header) {
	UdpHeader uh;
	Fifo fifo = { (void*)&uh, sizeof uh };
	fifoPutnetUdpHeader(&fifo,header);
	return fifoCalculateOnesComplementSumBigEndian(&fifo);
}

