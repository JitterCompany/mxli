#include <miimKs8721.h>
#include <fifoPrint.h>

bool fifoShowKs8721Configuration(Fifo *fifo, int addressPhy) {
	Ks8721_1fh info;
	miimRead(addressPhy,0x1f,&info.uint16);
	bool ok = true;
	switch(info.operationMode) {
		case KS8721_1FH_STILL_IN_AUTO_NEGOTIATION: ok = ok && fifoPrintString(fifo,"negotiating");
		break;
		case KS8721_1FH_10BASET_HALF_DUPLEX:	ok = ok && fifoPrintString(fifo,"10BASE-T half duplex");
		break;
		case KS8721_1FH_100BASETX_HALF_DUPLEX:	ok = ok && fifoPrintString(fifo,"100BASE-TX half duplex");
		break;
		case KS8721_1FH_10BASET_FULL_DUPLEX:		ok = ok && fifoPrintString(fifo,"10BASE-T full duplex");
		break;
		case KS8721_1FH_100BASETX_FULL_DUPLEX:	ok = ok && fifoPrintString(fifo,"100BASE-TX full duplex");
		break;
		case KS8721_1FH_PHY_MII_isolate:		ok = ok && fifoPrintString(fifo,"PHY/MII isolate");
		break;
	}
	if (info.energyDetect) ok = ok && fifoPrintString(fifo,",energy detect");
	if (info.enablePause) ok = ok && fifoPrintString(fifo,",PAUSE enabled");
	return fifoPrintString(fifo,"");
}

MiimLinkProperties miimGetLinkProperties(int addressPhy) {
	MiimLinkProperties link = { };
	Ks8721_1fh info;
	miimRead(addressPhy,0x1f,&info.uint16);
	switch(info.operationMode) {
		case KS8721_1FH_STILL_IN_AUTO_NEGOTIATION:	// link not valid 
		break;
		case KS8721_1FH_10BASET_HALF_DUPLEX:
			link.valid=1;
			link.speed100=0;
			link.fullDuplex=0;
			break;
		case KS8721_1FH_100BASETX_HALF_DUPLEX:
			link.valid=1;
			link.speed100=1;
			link.fullDuplex=0;
			break;
		case KS8721_1FH_10BASET_FULL_DUPLEX:
			link.valid=1;
			link.speed100=0;
			link.fullDuplex=1;
			break;
		case KS8721_1FH_100BASETX_FULL_DUPLEX:
			link.valid=1;
			link.speed100=1;
			link.fullDuplex=1;
			break;
	}
	link.pause = info.enablePause ? 1:0;
	return link;
}
