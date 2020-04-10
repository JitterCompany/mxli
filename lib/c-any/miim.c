#include <miim.h>
#include <simpleMath.h>
#include <fifoPrint.h>
#include <fatal.h>

#define DEBUG(x) x
#include <print.h>

#define show(status,x) fifoPrintString(fifo,separator) &&fifoPrintString(fifo,#x":") && fifoPrintUDec(fifo,status.x,1,5)

static void linkError(void) {
	fatal("miimLinkError.");
}

bool miimWrite(int phyAddress, int registerAddress, Uint16 value) __attribute__((weak,alias("linkError")));
bool miimRead(int phyAddress, int registerAddress, Uint16 *value) __attribute__((weak,alias("linkError")));

bool fifoPrintMiimBasicControl(Fifo *fifo, const MiimBasicControl control, const char *separator) {
	return	fifoPrintString(fifo,"MiimBasicControl")
		&& show(control,disableTransmitter)
		&& show(control,collisionTest)
		&& show(control,duplexMode)
		&& show(control,restartAutoNegotiation)
		&& show(control,isolate)
		&& show(control,powerDown)
		&& show(control,autoNegotiationEnable)
		&& show(control,speedSelection)
		&& show(control,loopBack)
		&& show(control,reset)
		;
}

bool fifoPrintMiimBasicStatus(Fifo *fifo, const MiimBasicStatus status, const char *separator) {
	return	fifoPrintString(fifo,"MiimBasicStatus")
		&& show(status,extendedCapability)
		&& show(status,jabberDetect)
		&& show(status,linkStatus)
		&& show(status,autoNegotiationAbility)
		&& show(status,remoteFault)
		&& show(status,autoNegotiationComplete)
		&& show(status,preambleSuppression)
		&& show(status,halfDuplex10BaseT)
		&& show(status,fullDuplex10BaseT)
		&& show(status,halfDuplex100BaseTx)
		&& show(status,fullDuplex100BaseTx)
		&& show(status,_100BaseT4)
		;
}

bool fifoPrintMiimLinkProperties(Fifo* fifo, const MiimLinkProperties p) {
	if (p.valid) return
		fifoPrintString(fifo, "speed=")
		&& fifoPrintString(fifo,p.speed100 ? "100" : "10")
		&& fifoPrintString(fifo,"Mb/s, duplex=")
		&& fifoPrintString(fifo,p.fullDuplex ? "full" : "half")
		&& fifoPrintString(fifo,", pause=")
		&& fifoPrintString(fifo,p.pause ? "yes" : "no")
		;
	else return fifoPrintString(fifo,"INVALID");
}

bool miimGetIdentifier(int addressPhy, MiimIdentifier *identifier) {
	MiimPhyIdentifier1 id1;
	MiimPhyIdentifier2 id2;

	if (miimRead(addressPhy,MIIM_IDENTIFIER1,&id1.uint16)
	&& miimRead(addressPhy,MIIM_IDENTIFIER2,&id2.uint16)) {
		identifier->oui	= id1.ouiBits17downto2<<6
				| id2.ouiBits23downto18
				;
		identifier->vendorModel = id2.vendorModel;
		identifier->modelRevision = id2.modelRevision;

		return true;
	}
	else return false;
}

static const char* ouiToString(int oui) {
	switch(oui) {
		case OUI_MICREL:			return "Micrel (reversed)";
		case OUI_NATIONAL_SEMICONDUCTOR:	return "National Semiconductor";
		default:				return "Unknown";
	}
}

bool fifoPrintMiimIdentifier(Fifo *fifo, const MiimIdentifier *id) {
	return fifoPrintString(fifo,"OUI=0x")
	&& fifoPrintHex(fifo,id->oui,6,6)
	&& fifoPrintString(fifo," (")
	&& fifoPrintString(fifo,ouiToString(id->oui))
	&& fifoPrintString(fifo,"), model=")
	&& fifoPrintUDec(fifo,id->vendorModel,1,2)
	&& fifoPrintString(fifo,", rev=")
	&& fifoPrintUDec(fifo,id->modelRevision,1,2);
}

int miimScanAddress(const MiimIdentifier *pattern) {
	MiimIdentifier identifier;

	if (pattern->oui==-1 && pattern->vendorModel==(char)-1 && pattern->modelRevision==(char)-1
	|| pattern==0) {
		// determine 'unaddressed' pattern: from 3 addresses that one, that occurs 2 times
		MiimIdentifier ids[3];
		MiimIdentifier idNone;
		for (int i=0; i<3; ++i) {
			if (!miimGetIdentifier(1+i,&ids[i])) return -1;
		}
		if (miimIdentifierEqual(&ids[0],&ids[1])
		|| miimIdentifierEqual(&ids[0],&ids[2]))	idNone = ids[0];
		else idNone = ids[1];
		for (int address=0; address<32; ++address) {
			DEBUG(
				printString("address="); printUDec32(address);
				printString(": oui=0x");
			)
			const bool gotId = miimGetIdentifier(address,&identifier);
			DEBUG(
				if (gotId) printHex32(identifier.oui);
				printLn();
			)
			if (gotId && !miimIdentifierEqual(&idNone,&identifier)) return address;
		}
		return -1;

	}
	else for (int address=0; address<32; ++address) {	// true pattern matching
		if (miimGetIdentifier(address,&identifier)
		&& (pattern->oui==-1 || pattern->oui==identifier.oui)
		&& (pattern->vendorModel==(char)-1 || pattern->vendorModel==identifier.vendorModel)
		&& (pattern->modelRevision==(char)-1 || pattern->modelRevision==identifier.modelRevision))
		return address;
	}
	return -1;
}

bool miimAutoNegotiationStart(int addressPhy) {
	MiimBasicStatus basicStatus;
	if (!miimRead(addressPhy,MIIM_BASIC_STATUS,&basicStatus.uint16)) return false;

	if (!basicStatus.autoNegotiationAbility || basicStatus.autoNegotiationComplete) return true;
	else {
		MiimBasicControl basicControl;
		if (!miimRead(addressPhy,MIIM_BASIC_CONTROL,&basicControl.uint16)) return false;
		basicControl.autoNegotiationEnable = 1;
		basicControl.restartAutoNegotiation = 1;
		if (!miimWrite(addressPhy,MIIM_BASIC_CONTROL,basicControl.uint16)) return false;
		else return true;
	}
}

bool miimAutoNegotiationCompleted(int addressPhy) {
	MiimBasicStatus basicStatus;
	if (!miimRead(addressPhy,MIIM_BASIC_STATUS,&basicStatus.uint16)) return false;

	return basicStatus.autoNegotiationComplete==1;
}

bool miimLinkLost(int addressPhy) {
	MiimBasicStatus basicStatus;
	if (!miimRead(addressPhy,MIIM_BASIC_STATUS,&basicStatus.uint16)) return false;

	return basicStatus.linkStatus==0;
}

bool fifoPrintMiimAna(Fifo *fifo, const MiimAna ana, const char *separator) {
	return	fifoPrintString(fifo,"MiimAna")
		&& show(ana,_10BaseT)
		&& show(ana,fullDuplex10BaseT)
		&& show(ana,_100BaseTx)
		&& show(ana,fullDuplex100BaseTx)
		&& show(ana,_100BaseT4)
		&& show(ana,pause)
		&& show(ana,remoteFault)
		&& show(ana,nextPage)
		;
}
