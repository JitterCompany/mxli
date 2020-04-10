//HEADER

#include <ip4Tcp.h>
#include <fifoParse.h>
#include <fifoPrint.h>
#include <simpleMath.h>


//SLICE
bool fifoPrintTcpOptions(Fifo *console, const TcpOptions *options) {
	const Uint8 *o = options->uint8s;
	fifoPrintString(console,"opt={ ");

	for (int i=0; i<sizeof options->uint8s; ) {
		if (o[i]==TCP_OPTION_EOL) {
			fifoPrintString(console,"EOL ");
			break;
		}
		else if (o[i]==TCP_OPTION_NOP) {
			fifoPrintString(console,"NOP ");
			i++;
		}
		else if (i+1<sizeof options->uint8s) {	// option with a length field
			const int length = o[i+1];
			if (i+length<=sizeof options->uint8s) {
				if (o[i]==TCP_OPTION_MSS) {
					if (length==4) {
						fifoPrintString(console,"MSS=");
						fifoPrintUDec(console,256*o[i+2] + o[i+3],1,5);
						fifoPrintChar(console,' ');
					}
					else {
						fifoPrintString(console,"MSS option broken");
						break;
					}
				}
				else {
					fifoPrintString(console,"?[");
					fifoPrintUDec(console,length,1,3);
					fifoPrintString(console,"] ");
				}
				i += length;
			}
			else {
				fifoPrintString(console,"<option length error> ");
				break;
			}
		}
		else {
			fifoPrintString(console,"<option length error> ");
			break;
		}
	}
	return fifoPrintString(console,"} ");
}

/*
bool fifoPrintTcpOptions(Fifo *console, const TcpOptions *options) {
	const Uint8 *o = options->uint8s;
	fifoPrintString(console,"opt={ ");

	for (int i=0; i<sizeof options->uint8s && o[i]!=TCP_OPTION_EOL; ) {
		if (o[i]==TCP_OPTION_NOP) {
			fifoPrintString(console,"NOP ");
			i++;
		}
		else if (o[i]==TCP_OPTION_MSS) {
			fifoPrintString(console,"MSS=");
			fifoPrintUDec(console,256*o[i+2] + o[i+3],1,5);
			fifoPrintChar(console,' ');
			i+=4;
		}
		else {
			fifoPrintString(console,"?[");
			i += o[i+1];
			fifoPrintUDec(console,o[i+1],1,3);
			fifoPrintString(console,"] ");
		}
	}
	return fifoPrintString(console,"} ");
}
*/

void tcpOptionsInit(TcpOptions *options) {
	memset(options->uint8s, TCP_OPTION_NOP, sizeof(TcpOptions));
}

size_t tcpOptionsSize(TcpOptions const *options) {
	int size = 0;
	const Uint8 *o = options->uint8s;
	for (int i=0; i<sizeof options->uint8s && o[i]!=TCP_OPTION_EOL; ) {
		switch(o[i]) {
			case TCP_OPTION_NOP: size++; i+=1; break;
			case TCP_OPTION_MSS: size+=4; i+=4; break;
			default: size += o[i+1]; i+=o[i+1]; // :o) this is very optimistic. Should cause an error instead.
		}
	}
	return (size+3) & ~3;
}

//SLICE
bool fifoPrintTcpHeader(Fifo *console, const TcpHeader *tcp, const TcpOptions *options) {
	return	fifoPrintString(console,"src=")
		&& fifoPrintUDec(console,tcp->source,1,5)
		&& fifoPrintString(console," dst=")
		&& fifoPrintUDec(console,tcp->destination,1,5)
		&& fifoPrintString(console," seq=")
		&& fifoPrintUDec(console,tcp->sequence,1,10)
		&& fifoPrintString(console," ack=")
		&& fifoPrintUDec(console,tcp->acknowledge,1,10)
		&& fifoPrintString(console," of4=")
		&& fifoPrintUDec(console,tcp->dataOffset4,1,3)
		&& fifoPrintString(console," ctl={ ")
		&& fifoPrintString(console,tcp->urg ? "URG " : "")
		&& fifoPrintString(console,tcp->ack ? "ACK " : "")
		&& fifoPrintString(console,tcp->psh ? "PSH " : "")
		&& fifoPrintString(console,tcp->syn ? "SYN " : "")
		&& fifoPrintString(console,tcp->fin ? "FIN " : "")
		&& fifoPrintString(console," }, win=")
		&& fifoPrintUDec(console,tcp->window,1,5)
		&& fifoPrintString(console," chksum=0x")
		&& fifoPrintHex(console,tcp->checksum,4,4)
		&& fifoPrintString(console," urg=")
		&& fifoPrintUDec(console,tcp->urgentPointer,1,5)
		&& fifoPrintChar(console,' ')
		&& fifoPrintTcpOptions(console,options)
		;
}

//SLICE
bool fifoGetnetTcpHeader(Fifo *inputStream, TcpHeader *tcp, TcpOptions *options) {
	if (sizeof *tcp <= fifoCanRead(inputStream)) {
		fifoGetnetUint16(inputStream,&tcp->source);
		fifoGetnetUint16(inputStream,&tcp->destination);
		fifoGetnetUint32(inputStream,&tcp->sequence);
		fifoGetnetUint32(inputStream,&tcp->acknowledge);
		fifoGetnetUint16(inputStream,&tcp->offsetAndFlags);
		fifoGetnetUint16(inputStream,&tcp->window);
		fifoGetnetUint16(inputStream,&tcp->checksum);
		fifoGetnetUint16(inputStream,&tcp->urgentPointer);

		tcpOptionsInit(options);
		const int oSize = tcp->dataOffset4*4 - sizeof(TcpHeader);
		return fifoGetN(inputStream,options,oSize);
	}
	else return false;
}

/** Writes a TCP Header into an output stream.
 * @param outputStream destination for the data
 * @param tcp the TCP header. If offset is zero, then this function calculates the offset field based on
 *   the options parameter. If offset is non-zero, then this value is used directly and MUST include the TCP options.
 * @param options the TCP options.
 * @param true in case of success, false if outputStream overflow.
 */
bool fifoPutnetTcpHeader(Fifo *outputStream, const TcpHeader *tcp, const TcpOptions *options) {
	const int oSize = tcp->dataOffset4 ? tcp->dataOffset4*4 - sizeof(TcpHeader) : tcpOptionsSize(options);
	const Uint32 offsetAndFlags = tcp->dataOffset4 ?
		tcp->offsetAndFlags
		:
		(sizeof(TcpHeader) + oSize) >> 2 | (tcp->offsetAndFlags & ~0x000F);	// calculated options size
	if (sizeof *tcp <= fifoCanWrite(outputStream))
		return	fifoPutnetInt16(outputStream,tcp->source)
			&& fifoPutnetInt16(outputStream,tcp->destination)
			&& fifoPutnetInt32(outputStream,tcp->sequence)
			&& fifoPutnetInt32(outputStream,tcp->acknowledge)
			&& fifoPutnetInt16(outputStream, offsetAndFlags)
			&& fifoPutnetInt16(outputStream,tcp->window)
			&& fifoPutnetInt16(outputStream,tcp->checksum)
			&& fifoPutnetInt16(outputStream,tcp->urgentPointer)
			&& fifoPutN(outputStream,options,oSize);
	else return false;
}

/*
bool fifoPutTcpPseudoHeader(Fifo *outputStream, const TcpPseudoHeader *header) {
	if (sizeof *header <= fifoCanWrite(outputStream)) {
		fifoPutIp4Address(outputStream, &header->sourceAddress);
		fifoPutIp4Address(outputStream, &header->destinationAddress);
		fifoPutInt8(outputStream, header->mbz);
		fifoPutInt8(outputStream, header->protocol);
		fifoPutnetInt16(outputStream, header->tcpLength);
		return true;
	}
	else return false;
}

Uint16 tcpPseudoHeaderChecksum(TcpPseudoHeader const *pseudoHeader) {
	TcpPseudoHeader uph;
	Fifo fifo = { (void*)&uph, sizeof uph };
	fifoPutTcpPseudoHeader(&fifo,pseudoHeader);
	return fifoCalculateOnesComplementSumBigEndian(&fifo);
}

Uint16 tcpHeaderChecksum(TcpHeader const* header) {
	TcpHeader uh;
	Fifo fifo = { (void*)&uh, sizeof uh };
	fifoPutTcpHeader(&fifo,header);
	return fifoCalculateOnesComplementSumBigEndian(&fifo);
}

*/
