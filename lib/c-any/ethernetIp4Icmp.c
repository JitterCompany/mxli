#include <ethernetIp4Icmp.h>

static void initIp4Header(Ip4Header *ip4Header) {
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

MessageResult ethernetIp4IcmpHandle(
	const EthernetIp4Environment *e,
	EthernetHeader const *ethernetHeader, Ip4Header const *ip4Header, Fifo *fifo) {

	IcmpHeader icmpHeader;
	if (fifoGetnetIcmpHeader(fifo,&icmpHeader)) {
		//DEBUG( fifoPrintIcmpHeader(stdout,&icmpHeader); )
		IcmpEcho icmpEcho;
		if (icmpHeader.type==ICMP_TYPE_ECHO_REQUEST && fifoGetnetIcmpEcho(fifo,&icmpEcho)) {
		//	DEBUG(
		//		fifoPrintChar(stdout,' ');
		//		fifoPrintIcmpEcho(stdout,&icmpEcho);
		//	)
			// generate response, if addressed to me
			if (ip4IsEqual(&ip4Header->destinationAddress,&e->configuration->ip4Address)
			&& e->ioFifo->canWrite()) {
				Fifo *response = e->ioFifo->writeBegin();
				EthernetHeader responseEthernetHeader = {
					.destinationAddress = ethernetHeader->sourceAddress, 
					.sourceAddress = e->configuration->ethernetAddress,
					.type = ETHERNET_TYPE_IP4,
				};

				Ip4Header responseIp4Header = {
					.sourceAddress = e->configuration->ip4Address,
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
				e->ioFifo->writeEnd();
				return success;
			}
		}
		// DEBUG(fifoPrintLn(stdout));
		return true;
	}
	return false;
}

