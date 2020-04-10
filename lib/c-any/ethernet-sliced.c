//HEADER
#include <ethernet.h>
#include <fifoParse.h>
#include <fifoPrint.h>

//SLICE
bool fifoPrintEthernetAddress(Fifo *fifo, const EthernetAddress* ethernetAddress) {
	return
		fifoPrintHex(fifo,ethernetAddress->int8s[0],2,2)
		&& fifoPrintChar(fifo,':')
		&& fifoPrintHex(fifo,ethernetAddress->int8s[1],2,2)
		&& fifoPrintChar(fifo,':')
		&& fifoPrintHex(fifo,ethernetAddress->int8s[2],2,2)
		&& fifoPrintChar(fifo,':')
		&& fifoPrintHex(fifo,ethernetAddress->int8s[3],2,2)
		&& fifoPrintChar(fifo,':')
		&& fifoPrintHex(fifo,ethernetAddress->int8s[4],2,2)
		&& fifoPrintChar(fifo,':')
		&& fifoPrintHex(fifo,ethernetAddress->int8s[5],2,2)
		;
}

//SLICE
bool fifoParseEthernetAddress(Fifo *fifo, EthernetAddress* ethernetAddress) {
	Fifo clone = *fifo;
	unsigned parsedAddress[6];
	if (fifoParseHexLimited(&clone,&parsedAddress[0],0,0xff)
	&& fifoParseExactChar(&clone,':')
	&& fifoParseHexLimited(&clone,&parsedAddress[1],0,0xff)
	&& fifoParseExactChar(&clone,':')
	&& fifoParseHexLimited(&clone,&parsedAddress[2],0,0xff)
	&& fifoParseExactChar(&clone,':')
	&& fifoParseHexLimited(&clone,&parsedAddress[3],0,0xff)
	&& fifoParseExactChar(&clone,':')
	&& fifoParseHexLimited(&clone,&parsedAddress[4],0,0xff)
	&& fifoParseExactChar(&clone,':')
	&& fifoParseHexLimited(&clone,&parsedAddress[5],0,0xff) ) {
		for (int i=0; i<6; ++i) ethernetAddress->int8s[i] = parsedAddress[i];
		fifoCopyReadPosition(fifo,&clone);
		return true;
	}
	else return false;
}

//SLICE
bool ethernetAddressIsEqual(const EthernetAddress *a1, const EthernetAddress *a2) {
	return	a1->int16s[0]==a2->int16s[0]
		&& a1->int16s[1]==a2->int16s[1]
		&& a1->int16s[2]==a2->int16s[2];
}

//SLICE
bool fifoGetnetEthernetHeader(Fifo *fifo, EthernetHeader *ethernetHeader) {
	if (sizeof *ethernetHeader <= fifoCanRead(fifo)) {
		fifoGetnetEthernetAddress(fifo,&ethernetHeader->destinationAddress);
		fifoGetnetEthernetAddress(fifo,&ethernetHeader->sourceAddress);
		fifoGetnetUint16(fifo,&ethernetHeader->type);
		return true;
	}
	else return false;
}

/** Extracts a non-VLAN Ethernet header. The remaining chars are the message body.
 * @param fifo the raw package.
 * @param header the output destination of the header.
 * @return true, if a valid Ethernet header was found, false otherwise (and fifo unchanged).
 */
/*
bool fifoGetEthernetHeader(Fifo *fifo, EthernetHeader *header) {
	Fifo clone = *fifo;
	if (fifoCanRead(&clone)>=sizeof (EthernetHeader)) {
		for (int i=0; i<sizeof(EthernetAddress); ++i)
			header->destinationAddress.int8s[i] = (Uint8)fifoRead(&clone);
		for (int i=0; i<sizeof(EthernetAddress); ++i)
			header->sourceAddress.int8s[i] = (Uint8)fifoRead(&clone);
		fifoGetnetUint16(&clone,&header->type);
		fifoCopyReadPosition(fifo,&clone);
		return true;
	}
	else return false;
}
*/

//SLICE
bool fifoPrintEthernetHeader(Fifo *fifo, EthernetHeader const *ethernetHeader) {
	return	fifoPrintString(fifo,"dst=")
		&& fifoPrintEthernetAddress(fifo,&ethernetHeader->destinationAddress)
		&& fifoPrintString(fifo,", src=")
		&& fifoPrintEthernetAddress(fifo,&ethernetHeader->sourceAddress)
		&& fifoPrintString(fifo,", type=0x")
		&& fifoPrintHex(fifo,ethernetHeader->type,4,4);
}

//SLICE
bool fifoPutnetEthernetHeader(Fifo *fifo, EthernetHeader const *ethernetHeader) {
	if (sizeof *ethernetHeader <= fifoCanWrite(fifo)) {
		fifoPutnetEthernetAddress(fifo,&ethernetHeader->destinationAddress);
		fifoPutnetEthernetAddress(fifo,&ethernetHeader->sourceAddress);
		fifoPutnetInt16(fifo,ethernetHeader->type);
		return true;
	}
	else return false;
}

//SLICE
const EthernetAddress ETHERNET_ADDRESS_BROADCAST = {{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }};

//SLICE
const EthernetAddress ETHERNET_ADDRESS_INVALID = {{ 0,0,0,0,0,0 }};
