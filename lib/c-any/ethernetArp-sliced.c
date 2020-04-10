//HEADER
#include <ethernetArp.h>
#include <fifoPrint.h>
#include <simpleMath.h>

//SLICE
bool fifoPrintEthernetArpHeader(Fifo *fifo, const EthernetArpHeader* h) {
	bool success = fifoPrintString(fifo,"ARP ");

	// check for 'normal' ARP packet
	const bool ok = h->hardwareAddressType==1	// Ethernet type
		&& h->protocol==0x0800			// 0x0800=IP
		&& h->hardwareAddressSize==6		// Ethernet address size
		&& h->protocolAddressSize==4		// IP4
		//&& (h->operation==1 || h->operation==2)	// request or response
		;

	if (ok) {
		switch(h->operation) {
			case 1: success = success && fifoPrintString(fifo,"request"); break;
			case 2: success = success && fifoPrintString(fifo,"reply"); break;
			case 3: success = success && fifoPrintString(fifo,"reverse request"); break;
			case 4: success = success && fifoPrintString(fifo,"reverse reply"); break;
			default: success = success && fifoPrintString(fifo,"opcode=")
						&& fifoPrintUDec(fifo,h->operation,1,2);
		}
		return success
		&& fifoPrintString(fifo," src=")
		&& fifoPrintEthernetAddress(fifo,&h->sourceEthernetAddress)
		&& fifoPrintChar(fifo,'/')
		&& fifoPrintIp4Address(fifo,&h->sourceIp4Address)
		&& fifoPrintString(fifo," dst=")
		&& fifoPrintEthernetAddress(fifo,&h->destinationEthernetAddress)
		&& fifoPrintChar(fifo,'/')
		&& fifoPrintIp4Address(fifo,&h->destinationIp4Address)
		;
	}
	else return success && fifoPrintString(fifo," <invalid>");
}

//SLICE
bool fifoGetnetEthernetArpHeader(Fifo *fifo, EthernetArpHeader *header) {
	Fifo clone = *fifo;
	if (fifoGetnetUint16(&clone,&header->hardwareAddressType)
	&& fifoGetnetUint16(&clone,&header->protocol)
	&& fifoGetUint8(&clone,&header->hardwareAddressSize)
	&& fifoGetUint8(&clone,&header->protocolAddressSize)
	&& fifoGetnetUint16(&clone,&header->operation)
	&& fifoGetnetEthernetAddress(&clone,&header->sourceEthernetAddress)
	&& fifoGetnetIp4Address(&clone,&header->sourceIp4Address)
	&& fifoGetnetEthernetAddress(&clone,&header->destinationEthernetAddress)
	&& fifoGetnetIp4Address(&clone,&header->destinationIp4Address)) {
		fifoCopyReadPosition(fifo,&clone);
		return true;
	}
	else return false;
}

//SLICE
bool fifoPutnetEthernetArpHeader(Fifo *fifo, const EthernetArpHeader *header) {
	if (sizeof *header <= fifoCanWrite(fifo)) {
		fifoPutnetInt16(fifo,header->hardwareAddressType);
		fifoPutnetInt16(fifo,header->protocol);
		fifoPutInt8(fifo,header->hardwareAddressSize);
		fifoPutInt8(fifo,header->protocolAddressSize);
		fifoPutnetInt16(fifo,header->operation);
		fifoPutnetEthernetAddress(fifo,&header->sourceEthernetAddress);
		fifoPutnetIp4Address(fifo,&header->sourceIp4Address);
		fifoPutnetEthernetAddress(fifo,&header->destinationEthernetAddress);
		fifoPutnetIp4Address(fifo,&header->destinationIp4Address);
		return false;
	}
	else return false;
}

//SLICE
EthernetArpCacheEntry* ethernetArpCacheLookup(EthernetArpCache* cache, const Ip4Address *ip4Address,
	int currentTime) {
	for (int a=0; a<cache->elements; ++a)
		if (cache->table[a].state & ETHERNET_ARP_RESOLVED	// resolved bit set
		&& cache->table[a].ip4Address.int32==ip4Address->int32) {
			cache->table[a].timeLookup = currentTime;
			return cache->table+a;
		}

	// not found, so install a 'REQUESTED' entry
	EthernetArpCacheEntry *entry = ethernetArpCacheLru(cache, ip4Address, currentTime);
	entry->ip4Address = *ip4Address;
	entry->state = ETHERNET_ARP_UNRESOLVED;
	entry->time = currentTime;
	return 0;
}

//SLICE
const Ip4Address* ethernetArpCacheNextRefresh(EthernetArpCache* cache, int time) {
	// first the unresolved ones
	for (int a=0; a<cache->elements; ++a) {
		// initial request
		EthernetArpCacheEntry *entry = &cache->table[a];
		if (entry->state==ETHERNET_ARP_UNRESOLVED) {
			entry->state = ETHERNET_ARP_REQUESTED;
			return &entry->ip4Address;
		}
	}
	// later the ones to refresh
	for (int a=0; a<cache->elements; ++a) {
		// periodic refresh request
		EthernetArpCacheEntry *entry = &cache->table[a];
		if (cache->table[a].state==ETHERNET_ARP_RESOLVED
		&& (time-entry->timeLookup > cache->entryRefreshTime)) {
			entry->state = ETHERNET_ARP_RESOLVED_REQUESTED;
			return &entry->ip4Address;
		}
	}
	return 0;
}

//SLICE
void ethernetArpCacheExpire(EthernetArpCache* cache, int currentTime) {
	for (int a=0; a<cache->elements; ++a)
		if (cache->table[a].state!=ETHERNET_ARP_INVALID
		&& cache->table[a].time+cache->entryLifeTime < currentTime)
			cache->table[a].state=ETHERNET_ARP_INVALID;
}

//SLICE
EthernetArpCacheEntry* ethernetArpCacheLru(EthernetArpCache *cache, const Ip4Address* ip4Address, int currentTime) {
	if (ip4Address!=0) for (int a=0; a<cache->elements; ++a)
		if (cache->table[a].state!=ETHERNET_ARP_INVALID
		&& ip4IsEqual(&cache->table[a].ip4Address,ip4Address)) return cache->table+a;

	// no exact match
	int eldestIndex = 0;
	int eldestTime = currentTime;
	for (int a=0; a<cache->elements; ++a) {
		if (cache->table[a].state==ETHERNET_ARP_INVALID) {	// first free entry
			return cache->table+a;
		}
		else {	// valid entry
			if (cache->table[a].timeLookup<eldestTime) { 	// check if it's even older
				eldestTime = cache->table[a].timeLookup;
				eldestIndex = a;
			}
		}
	}
	return cache->table+eldestIndex;	// replace an existing one
}

//SLICE
void ethernetArpCacheSet(EthernetArpCache* cache, const Ip4Address* ip4Address, const EthernetAddress *ethernetAddress,
	int currentTime) {
	EthernetArpCacheEntry* entry = ethernetArpCacheLru(cache,ip4Address,currentTime);

	entry->ip4Address = *ip4Address;
	entry->ethernetAddress = *ethernetAddress;
	entry->time = currentTime;
	entry->timeLookup = currentTime;
	entry->state = ETHERNET_ARP_RESOLVED;
}

//SLICE
bool fifoPrintEthernetArpCacheEntry(Fifo *output, const EthernetArpCacheEntry *entry) {
	fifoPrintIp4Address(output,&entry->ip4Address);
	fifoPrintString(output,"-->");
	fifoPrintEthernetAddress(output,&entry->ethernetAddress);
	fifoPrintString(output," Tw=");
	fifoPrintSDec(output,entry->time,1,10,false);
	fifoPrintString(output," Tu=");
	fifoPrintSDec(output,entry->timeLookup,1,10,false);
	fifoPrintChar(output,' ');
	switch(entry->state) {
		case ETHERNET_ARP_INVALID:		return fifoPrintString(output,"INVALID");
		case ETHERNET_ARP_UNRESOLVED:		return fifoPrintString(output,"UNRESOLVED");
		case ETHERNET_ARP_REQUESTED:		return fifoPrintString(output,"REQUESTED");
		case ETHERNET_ARP_RESOLVED:		return fifoPrintString(output,"RESOLVED");
		case ETHERNET_ARP_RESOLVED_REQUESTED:	return fifoPrintString(output,"RESOLVED+REQUESTED");
		default:				return fifoPrintString(output,"ERROR");
	}
}

//SLICE
bool fifoPrintEthernetArpCache(Fifo *output, const EthernetArpCache *cache, bool printAll) {
	bool success = fifoPrintString(output,"ARP cache dump:\n");
	for (int e=0; e<cache->elements; ++e) {
		if (cache->table[e].state!=ETHERNET_ARP_INVALID || printAll) {
			fifoPrintString(output,"  #");
			fifoPrintUDec(output,e,2,9);
			fifoPrintChar(output,' ');
			fifoPrintEthernetArpCacheEntry(output,&cache->table[e]);
			success = success && fifoPrintLn(output);
		}
	}
	return success;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ARP handler

//SLICE
bool ethernetArpIp4Resolve(const EthernetArpIp4Configuration *configuration, Ip4Address const *address) {
	if (configuration->ioFifo->canWrite()) {
		Fifo *writer = configuration->ioFifo->writeBegin();
		EthernetHeader ethernetHeaderReply = {
			.destinationAddress = ETHERNET_ADDRESS_BROADCAST,
			.sourceAddress = * configuration->ethernetAddress,
			.type = ETHERNET_TYPE_ARP
		};
		fifoPutnetEthernetHeader(writer,&ethernetHeaderReply);

		const EthernetArpHeader ethernetArpHeaderReply = {
			.hardwareAddressType = 1,
			.hardwareAddressSize = sizeof (EthernetAddress),
			.protocol = ETHERNET_TYPE_IP4,
			.protocolAddressSize = sizeof (Ip4Address),
			.operation = ETHERNET_ARP_OP_REQUEST,
			.sourceEthernetAddress = * configuration->ethernetAddress,
			.sourceIp4Address = * configuration->ip4Address,
			.destinationEthernetAddress = ETHERNET_ADDRESS_INVALID,
			.destinationIp4Address = *address	// the one to resolve
		};
		fifoPutnetEthernetArpHeader(writer,&ethernetArpHeaderReply);
		fifoPrintString(writer,"FC200 ARP query.");

		configuration->ioFifo->writeEnd();
		return true;
	}
	else return false;
}

//SLICE
bool ethernetArpIp4ResolvePending(const EthernetArpIp4Configuration *configuration, int time) {
	const Ip4Address *address = ethernetArpCacheNextRefresh(configuration->ethernetArpCache,time);
	if (address!=0) return ethernetArpIp4Resolve(configuration,address);
	else return false;
}

//SLICE
MessageResult ethernetArpIp4Handle(
	const EthernetArpIp4Configuration	*configuration,
	const EthernetHeader			*ethernetHeader,
	Fifo					*message,
	int					time
) {
	Fifo clone = *message;

	EthernetArpHeader ethernetArpHeader;
	if (fifoGetnetEthernetArpHeader(&clone,&ethernetArpHeader)) {
		// ARP request for this unit
		if (ip4IsEqual(configuration->ip4Address,&ethernetArpHeader.destinationIp4Address)) {
			if (configuration->ioFifo->canWrite()) {
				Fifo *writer = configuration->ioFifo->writeBegin();
				EthernetHeader ethernetHeaderReply = {
					.destinationAddress = ethernetHeader->sourceAddress,
					.sourceAddress = * configuration->ethernetAddress,
					.type = ETHERNET_TYPE_ARP
				};
				fifoPutnetEthernetHeader(writer,&ethernetHeaderReply);

				const EthernetArpHeader ethernetArpHeaderReply = {
					.hardwareAddressType = 1,
					.hardwareAddressSize = sizeof (EthernetAddress),
					.protocol = ETHERNET_TYPE_IP4,
					.protocolAddressSize = sizeof (Ip4Address),
					.operation = ETHERNET_ARP_OP_REPLY,
					.sourceEthernetAddress = * configuration->ethernetAddress,
					.sourceIp4Address = * configuration->ip4Address,
					.destinationEthernetAddress = ethernetArpHeader.sourceEthernetAddress,
					.destinationIp4Address = ethernetArpHeader.sourceIp4Address
				};
				fifoPutnetEthernetArpHeader(writer,&ethernetArpHeaderReply);
				fifoPrintString(writer,"FC200 ARP reply.");

				configuration->ioFifo->writeEnd();
				// put the caller into the ARP cache?
				ethernetArpCacheSet(configuration->ethernetArpCache,
					&ethernetArpHeader.sourceIp4Address, &ethernetArpHeader.sourceEthernetAddress,time);
			}
			else return MESSAGE_POSTPONE;	// no transmission packet available
		}
		// ARP reply back to this unit
		else if (ethernetAddressIsEqual(configuration->ethernetAddress, & ethernetArpHeader.destinationEthernetAddress)
		&& ethernetArpHeader.operation==ETHERNET_ARP_OP_REPLY) {
			ethernetArpCacheSet(configuration->ethernetArpCache,
				&ethernetArpHeader.sourceIp4Address, &ethernetArpHeader.sourceEthernetAddress,time);
		}
	}
	return MESSAGE_DONE;
}

