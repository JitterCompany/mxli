/*
  isp.c - main program of isp, a NXP LPC ARM controller isp programmer for Linux.
  Copyright 2011-2013 Marc Prager
 
  isp is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  isp is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with isp.
  If not see <http://www.gnu.org/licenses/>
 */

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <time.h>		// high resolution timers

// c-any library headers
#include <fifoParse.h>
#include <fifoParseStructure.h>
#include <fifoPopt.h>
#include <fifoPrint.h>
#include <lpcMemories.h>
#include <uu.h>
#include <macros.h>

#include <c-linux/serial.h>
#include <c-linux/hexFile.h>

/* History:
 * Version 1.0 (09/2011)
 * ======================
 * - full functionality
 *
 * Version 1.1 (04/2012)
 * ======================
 * - added Intel hex-file support, although in a not very clean programming style.
 * - put under GPL (03/2013)
 *
 * Version 1.2 (07/2013)
 * ======================
 * - added support for LPC800
 *
 * Version 1.2+
 * =============
 * - BIIIIG BUG fix: checksum in vector 5 (reserved), not in vector 7 (FIQ).
 * - still not correct (the above!)
 * - added -u (do not use 'echo off') command line switch.
 */
#define ISP_VERSION_COPYRIGHT "1.2 ($Rev: 84 $) (C) Marc Prager 2011-2013."

static inline bool lpcReset(int fd, bool active) {
	return serialSetDtr(fd,!active);
}

static inline bool lpcBsl(int fd, bool active) {
	return serialSetRts(fd,!active);
}

static char outBuffer[1000];
static char inBuffer[1000];
Fifo fifoOut = { outBuffer, sizeof outBuffer };
Fifo fifoIn = { inBuffer, sizeof inBuffer };
static bool useEcho = false;

bool fifoScanExactString(Fifo *input, const char *word) {
	int length = strlen(word);
	while (fifoCanRead(input)>=length) {
		if (fifoParseExactString(input,word)) return true;
		else {
			if (fifoCanRead(input)) fifoRead(input);	// advance one char
			else return false;
		}
	}
	return false;
}

/** Scans for a message of the incoming stream.
 */
bool ioScanExactString(int fd, const char *word) {
	char c;
	while (1==read(fd,&c,1) && fifoPrintChar(&fifoIn,c)) if (fifoScanExactString(&fifoIn,word)) return true;
	return false;	// timeout reached before match
}

#ifdef DEBUG_OUT
bool ioDump(int fd) {
	char c;
	while (1==read(fd,&c,1)) switch(c) {
		case '\n':	printf("\\n\n"); break;
		case '\r':	printf("\\r"); break;
		default: putchar(c);
	}
	putchar('\n');
	return true;
}

//Appends line end sequence, sends fifoOut, receives status code and checks if it's 0.
bool ioSendAndDump(int fd) {
	return	fifoPrintString(&fifoOut,"\r\n")
		&& ioWriteFifo(fd,&fifoOut)
		&& ioDump(fd);
}
#endif

bool fifoDump(Fifo *fifo) {
	char c;
	while (fifoCanRead(fifo)) switch(c=fifoRead(fifo)) {
		case '\n':	fprintf(stderr,"\\n\n"); break;
		case '\r':	fprintf(stderr,"\\r"); break;
		default: fprintf(stderr,"%c",c);
	}
	fprintf(stderr,"\n");
	fflush(stderr);
	return true;
}

bool ioWriteString(int fd, const char *data) {
	return strlen(data) == write(fd,data,strlen(data));
}

bool ioWriteFifo(int fd, Fifo *fifo) {
	while (fifoCanRead(fifo)) {
		const char c = fifoRead(fifo);
		if (1!=write(fd,&c,1)) return false;
	}
	return true;
}

bool lpcSync(int fd, int crystalHz) {
	if (!ioWriteString(fd,"?")
	|| !ioScanExactString(fd,"Synchronized\r\n")) return false;

	if (!ioWriteString(fd,"Synchronized\r\n")
	|| !ioScanExactString(fd,"OK\r\n")) return false;

	if (crystalHz > 0) {
		fifoPrintUDec(&fifoOut,(crystalHz+500)/1000,1,10); fifoPrintString(&fifoOut,"\r\n");
		if (!ioWriteFifo(fd,&fifoOut)
		|| !ioScanExactString(fd,"OK\r\n")) return false;
	}

	return true;
}

const char* returnMessage(int returnCode) {
	switch(returnCode) {
		case 0:		return "CMD_SUCCESS";
		case 1:		return "INVALID_COMMAND";
		case 2:		return "SRC_ADDR_ERROR";
		case 3:		return "DST_ADDR_ERROR";
		case 4:		return "SRC_ADDR_NOT_MAPPED";
		case 5:		return "DST_ADDR_NOT_MAPPED";
		case 6:		return "COUNT_ERROR";
		case 7:		return "INVALID_SECTOR";
		case 8:		return "SECTOR_NOT_BLANK";
		case 9:		return "SECTOR_NOT_PREPARED";
		case 10:	return "COMPARE_ERROR";
		case 11:	return "BUSY";
		case 12:	return "PARAM_ERROR";
		case 13:	return "ADDR_ERROR";
		case 14:	return "ADDR_NOT_MAPPED";
		case 15:	return "CMD_LOCKED";
		case 16:	return "INVALID_CODE";
		case 17:	return "INVALID_BAUD_RATE";
		case 18:	return "INVALID_STOP_BIT";
		case 19:	return "CODE_READ_PROTECTION";
		default:	return "UNDEFINED ERROR";
	}
}

bool ioScanLine(int fd, Fifo *line) {
	char c;
	while (1==read(fd,&c,1)) {
		fprintf(stderr,"{%c=0x%02X}",c,c & 0xFF ); fflush(stderr);
		switch(c) {
			case 0x13:	// flow control: stop
			case 0x11:	// flow control: start
				fprintf(stderr,"Flow control not implemented :-(\n");
				return false;
			default:
				if (!fifoPrintChar(&fifoIn,c)) return false;
				if (fifoParseLine(&fifoIn,line,EOL_CRLF)) return true;
		}
	}
	fprintf(stderr,"Serial timeout.\n");
	return false;
}

int ioScanResult(int fd) {
	Fifo line;
	if (useEcho && (!ioScanLine(fd,&line))) {
		fprintf(stderr,"Input buffer dump [%lu]:",fifoCanRead(&fifoIn));
		fifoDump(&fifoIn);
		return -1;
	}

	if (ioScanLine(fd,&line)) {
		int returnCode;
		if (fifoParseInt(&line,&returnCode)) {
			if (returnCode!=0) fprintf(stderr,"return code: %s\n",returnMessage(returnCode));
			return returnCode;
		}
		else {
			fprintf(stderr,"Input buffer dump [%lu]:",fifoCanRead(&fifoIn));
			fifoDump(&fifoIn);
			return -1;
		}
	}
	else {
		fprintf(stderr,"Input buffer dump [%lu]:",fifoCanRead(&fifoIn));
		fifoDump(&fifoIn);
		return -1;
	}
}

bool ioScanInt(int fd, int* value) {
	Fifo line;
	return ioScanLine(fd,&line) && fifoParseInt(&line,value);
}

bool ioScanString(int fd, const char *string) {
	Fifo line;
	if (ioScanLine(fd,&line)) {
		Fifo clone = line;
		if (fifoScanExactString(&line,string)) return true;
		else {
			fprintf(stderr,"Unexpected: ");
			fflush(stderr);
			ioWriteFifo(1,&clone);
			return false;
		}
	}
	else return false;
}

/** Appends line end sequence, sends fifoOut, receives status code and checks if it's 0.
 */
bool ioSendAndCheck(int fd) {
	return	fifoPrintString(&fifoOut,"\r\n")
		&& ioWriteFifo(fd,&fifoOut)
		&& 0==ioScanResult(fd);
}

// At least LPC17 answers A 0\r0\r\n on A 0\r\n. A \n is missing.
// Therefore we use only \r in this command.
// We turn echo off quickly to get rid of this shit.
bool lpcEcho(int fd, bool on) {
	const char *command = on ? "A 1\r" : "A 0\r"; 
	return	ioWriteString(fd,command)
		&& ioScanExactString(fd,command) && ioScanResult(fd)==0;
}

bool lpcCommandIntInt(int fd, char commandChar, int a, int b) {
	return	fifoPrintChar(&fifoOut,commandChar)
		&& fifoPrintChar(&fifoOut,' ')
		&& fifoPrintUDec(&fifoOut,a,1,10)
		&& fifoPrintChar(&fifoOut,' ')
		&& fifoPrintUDec(&fifoOut,b,1,10)
		&& ioSendAndCheck(fd);
}

bool lpcBaud(int fd, int baud, int stopBits) {
	return lpcCommandIntInt(fd,'B',baud,stopBits);
}

bool lpcReadPartId(int fd, int *id) {
	return 	fifoPrintChar(&fifoOut,'J')
		&& ioSendAndCheck(fd)
		&& ioScanInt(fd,id);
}

bool lpcReadBootCodeVersion(int fd, int *version) {
	int major,minor;
	if (fifoPrintChar(&fifoOut,'K')
	&& ioSendAndCheck(fd)
	&& ioScanInt(fd,&major)
	&& ioScanInt(fd,&minor)) {
		*version = major<<8 | minor;
		return true;
	}
	else return false;
}

/** Reads out the device serial number.
 * @param sn 4 32-bit integers.
 */
bool lpcReadSerialNumber(int fd, int *sn) {
	return	ioWriteString(fd,"N\r\n")
		&& 0==ioScanResult(fd)
		&& ioScanInt(fd,sn+0)
		&& ioScanInt(fd,sn+1)
		&& ioScanInt(fd,sn+2)
		&& ioScanInt(fd,sn+3);
}

bool lpcCopyRamToFlash(int fd, unsigned addressFlash, unsigned addressRam, int n) {
	return	fifoPrintString(&fifoOut,"C ")
		&& fifoPrintUDec(&fifoOut,addressFlash,1,10)
		&& fifoPrintChar(&fifoOut,' ')
		&& fifoPrintUDec(&fifoOut,addressRam,1,10)
		&& fifoPrintChar(&fifoOut,' ')
		&& fifoPrintUDec(&fifoOut,n,1,10)
		&& ioSendAndCheck(fd);
}

bool lpcGo(int fd, unsigned pc, bool thumbMode) {
	return	fifoPrintString(&fifoOut,"G ")
		&& fifoPrintUDec(&fifoOut,pc,1,10)
		&& fifoPrintChar(&fifoOut,' ')
		&& fifoPrintChar(&fifoOut,thumbMode ? 'T' : 'A')
		&& ioSendAndCheck(fd);
}

bool lpcPrepareForWrite(int fd, int sectorStart, int sectorEnd) {
	return	lpcCommandIntInt(fd,'P',sectorStart,sectorEnd);
}

bool lpcErase(int fd, int sectorStart, int sectorEnd) {
	fprintf(stderr,"Erasing sectors %d to %d NOW\n",sectorStart,sectorEnd);
	return	lpcCommandIntInt(fd,'E',sectorStart,sectorEnd);
}

bool lpcUnlock(int fd) {
	return	fifoPrintString(&fifoOut,"U 23130")
		&& ioSendAndCheck(fd);
}

bool lpcBlankCheck(int fd, int sectorStart, int sectorEnd) {
	int offset, value;
	bool success = 
		fifoPrintString(&fifoOut,"I ")
		&& fifoPrintUDec(&fifoOut,sectorStart,1,10)
		&& fifoPrintChar(&fifoOut,' ')
		&& fifoPrintUDec(&fifoOut,sectorEnd,1,10)
		&& fifoPrintString(&fifoOut,"\r\n")
		&& ioWriteFifo(fd,&fifoOut);

	if (success) switch(ioScanResult(fd)) {
		case 0:	return true;
		case 8:	// SECTOR_NOT_BLANK
			if (ioScanInt(fd,&offset)
			&& ioScanInt(fd,&value)) ;	// OK.
			else {
				fprintf(stderr,"Communication error.");
			}
		default:
			return false;
	}
	else return false;
}

bool lpcCompare(int fd, Uint32 addrA, Uint32 addrB, int n) {
	return	fifoPrintString(&fifoOut,"M ")
		&& fifoPrintUDec(&fifoOut,addrA,1,10)
		&& fifoPrintChar(&fifoOut,' ')
		&& fifoPrintUDec(&fifoOut,addrB,1,10)
		&& fifoPrintChar(&fifoOut,' ')
		&& fifoPrintUDec(&fifoOut,n,1,10)
		&& ioSendAndCheck(fd);
}

bool lpcBailOut(int fd) {
	ioWriteString(fd,"\x1B");
	return false;
}

bool lpcReadUuencode(int fd, Fifo *data, unsigned address, int n) {
	const int r0 = fifoCanRead(data);

	if (lpcCommandIntInt(fd,'R',address,n)) {
		int checksum=0;
		for (int lineNo=0; fifoCanRead(data)-r0 <n; lineNo++) {
			Fifo fifoLine;
			if (ioScanLine(fd,&fifoLine) && fifoUuDecodeLine(data,&fifoLine,&checksum)) ;	// fine
			else return lpcBailOut(fd);

			if ((lineNo % 20)==19 || fifoCanRead(data)-r0 ==n) {
				int checksumLpc;
				if (ioScanInt(fd,&checksumLpc)
				&& checksum==checksumLpc) {
					fprintf(stderr,".");
					ioWriteString(fd,"OK\r\n");	// :o) unchecked
				}
				else {
					fprintf(stderr,"Checksum Error.\n");
					return lpcBailOut(fd);	// we do not retry
				}
				checksum = 0;
			}
		}
		return true;
	}
	else return false;
}

bool lpcReadBinary(int fd, Fifo *data, unsigned address, int n) {
	//const int r0 = fifoCanRead(data);

	if (lpcCommandIntInt(fd,'R',address,n)) {
		// :o) difficult mode switching line-oriented <=> binary
	/*
		for (int b=0; fifoCanRead(data)-r0 <n; lineNo++) {
			Fifo fifoLine;
			if (ioScanLine(fd,&fifoLine) && fifoUuDecodeLine(data,&fifoLine,&checksum)) ;	// fine
			else return lpcBailOut(fd);

			if ((lineNo % 20)==19 || fifoCanRead(data)-r0 ==n) {
				int checksumLpc;
				if (ioScanInt(fd,&checksumLpc)
				&& checksum==checksumLpc) {
					fprintf(stderr,".");
					ioWriteString(fd,"OK\r\n");	// :o) unchecked
				}
				else {
					fprintf(stderr,"Checksum Error.\n");
					return lpcBailOut(fd);	// we do not retry
				}
				checksum = 0;
			}
		}
	*/
		return true;
	}
	else return false;
}

bool lpcRead(int fd, int ispData, Fifo *data, unsigned address, int n) {
	switch(ispData) {
		case ISP_DATA_UUENCODE: return lpcReadUuencode(fd,data,address,n);
		case ISP_DATA_BINARY: return lpcReadBinary(fd,data,address,n);
		default: return lpcBailOut(fd);
	}
}

bool lpcWriteUuencode(int fd, unsigned address, Fifo *data) {
fprintf(stderr,"lpcWriteUuencode()");
	if (lpcCommandIntInt(fd,'W',address,fifoCanRead(data))) {
		int checksum=0;
		for (int lineNo=0; fifoCanRead(data); lineNo++) {
			if (fifoUuEncodeLine(&fifoOut,data,&checksum)) {
				//fprintf(stderr,"line %d\n",lineNo);
				ioWriteFifo(fd,&fifoOut);
			}
			else return lpcBailOut(fd);

			if ((lineNo%20)==19 || !fifoCanRead(data)) {	// checksum
				fprintf(stderr,".");
				fflush(stderr);
				if (fifoPrintUDec(&fifoOut,checksum,1,10)
				&& fifoPrintString(&fifoOut,"\r\n")
				&& ioWriteFifo(fd,&fifoOut)
				&& ioScanString(fd,"OK")) {
					//fprintf(stderr,"Checksum confirmed\n");
				}
				else {
					fprintf(stderr,"Checksum error.\n");
					return lpcBailOut(fd);
				}

				checksum = 0;
			}
		}
		return true;
	}
	else return false;
}

bool lpcWriteBinary(int fd, unsigned address, Fifo *data) {
fprintf(stderr,"lpcWriteBinary()");
	const int n = fifoCanRead(data);
	if (lpcCommandIntInt(fd,'W',address,n)) {
		fifoAppend(&fifoOut,data);
		fprintf(stderr,".");
		fflush(stderr);
		return	ioWriteFifo(fd,&fifoOut) && ioScanString(fd,"OK")
			|| lpcBailOut(fd);
	}
	else return false;
}

bool lpcWrite(int fd, int ispData, unsigned address, Fifo *data) {
	switch(ispData) {
		case ISP_DATA_UUENCODE: return lpcWriteUuencode(fd,address,data);
		case ISP_DATA_BINARY: return lpcWriteBinary(fd,address,data);
		default: return lpcBailOut(fd);
	}
}

bool lpcWriteN(int fd, int ispData, unsigned address, Fifo *data, int n) {
	Fifo fifoFraction;
	return	fifoParseN(data,&fifoFraction,n)
		&& lpcWrite(fd,ispData,address,&fifoFraction);
}

typedef struct {
	bool 	verify;			///< verify flash after copy RAM to FLASH
	bool	fast;			///< waste FLASH lifetime for speed
	int	ramLowerSpace;		///< ISP uses 0x200 bytes at bottom of RAM
	int	ramUpperSpace;		///< ISP/IAP uses 32 + 0x100 bytes at top of RAM
} LpcProgramConfig;

/** Extended programming function for hex Images.
 *  @param fast erase sectors in one call, if not all blank. Otherwise erase sector for sector.
 * code read protection
 */
bool lpcProgramNew(int fd, LpcFamilyMember const *member, const HexImage *hexImage, const LpcProgramConfig *conf) {
	const int blockLimit = member->sizeRamKs[0]*1024 - conf->ramLowerSpace - conf->ramUpperSpace;
	const LpcFamilyMemoryMap *map = member->familyMemoryMap;
	const int addressRam = map->addressRams[0] + conf->ramLowerSpace;
	fprintf(stderr,"Transfer RAM: %dkiB+%dB @ 0x%08X\n",blockLimit/1024,blockLimit%1024,addressRam);

	if (hexImageSegments(hexImage)!=1) {
		fprintf(stderr,"Cannot handle more than one segment so far, yet.\n");
		return false;
	}
	const int imageSize = hexImage->segments[0].size;
	const int address = hexImage->segments[0].address;

	// copy image to a Fifo
	char imageBuffer[512*1024];
	Fifo fifoImage = { imageBuffer, sizeof imageBuffer };
	for (int b=0; b<imageSize; b++) fifoWrite(&fifoImage,hexImage->segments[0].data[b]);

	// optimized for big images => maximum block with 'Copy RAM to FLASH' unless image is really small.
	int blockSize = 0;
	for (int i=0; i<ELEMENTS(map->blockSizes) && map->blockSizes[i]>0; ++i) {
		if (map->blockSizes[i] <= blockLimit
		&& blockSize<imageSize) 	blockSize = map->blockSizes[i];
	}
	// pad fifo for this block size
	while (fifoCanRead(&fifoImage) % blockSize) if (!fifoPrintChar(&fifoImage,0xFF)) return false;

	const int blocks = fifoCanRead(&fifoImage) / blockSize;
	const int blocksAtOnce = blockLimit / blockSize;	// the will be 1 at least

	fprintf(stderr,"Transmission RAM: %dblocks (%dkiB), units of %dB\n",
		blocksAtOnce, blocksAtOnce*blockSize/1024, blockSize);
	fflush(stderr);

	fprintf(stderr,"Flash address range 0x%08x..0x%08x, ",address,address+blockSize*blocks-1);
	int sectorFrom, sectorTo;
	if (lpcAddressRangeToSectorRange(member,&sectorFrom,&sectorTo,address,address+blockSize*blocks-1)) {
		fprintf(stderr,"sectors: %d..%d\n",sectorFrom,sectorTo);
	}
	else {
		fprintf(stderr,"image does not fit into FLASH\n");
		return false;
	}

	if (!lpcUnlock(fd)) {
		fprintf(stderr,"Failed to unlock device.\n");
		return false;
	}

	if (!lpcBlankCheck(fd,sectorFrom,sectorTo)) {	// erasing required
		if (conf->fast) {
			if (lpcPrepareForWrite(fd,sectorFrom,sectorTo)
			&& lpcErase(fd,sectorFrom,sectorTo)) fprintf(stderr,"Erased en bloc.\n");
			else {
				fprintf(stderr,"Failed to erase sectors %d to %d\n",sectorFrom,sectorTo);
				return false;
			}
		}
		else for (int s=sectorFrom; s<=sectorTo; ++s) {
			if (lpcBlankCheck(fd,s,s)
			|| lpcPrepareForWrite(fd,s,s) && lpcErase(fd,s,s)) ;	// fine
			else {
				fprintf(stderr,"Failed to erase sector %d\n",s);
				return false;
			}
		}
	}

	for (int block=0; block<blocks; ) {
		// transfer bytes
		for (int subBlock=0; subBlock<blocksAtOnce && block+subBlock<blocks; ++subBlock) {
			fprintf(stderr,"\nTransfer block %d to RAM ",block+subBlock);
			if (lpcWriteN(fd,member->ispData,addressRam+subBlock*blockSize,&fifoImage,blockSize)) {	// fine
				fprintf(stderr,"OK.");
			}
			else {
				fprintf(stderr,"FAILED!\n");
				return false;
			}
		}
		// do flash programming
		for (int subBlock=0; subBlock<blocksAtOnce && block+subBlock<blocks; ++subBlock) {
			const Uint32 addressFlash = address+(block+subBlock)*blockSize;
			const int sector = lpcAddressToSector(member,addressFlash);
			fprintf(stderr,"\nTransfer block %d to FLASH, sector %d ",block+subBlock,sector);
			if (lpcPrepareForWrite(fd,sector,sector)
			&& lpcCopyRamToFlash(fd, addressFlash, addressRam+subBlock*blockSize, blockSize)); // fine
			else {
				fprintf(stderr,"Failed to Copy RAM to FLASH.\n");
				return false;
			}
			if (conf->verify) {
				if (addressFlash==0) {
					fprintf(stderr," verify not possible for sector 0.");
				}
				else {
					if (lpcCompare(fd,addressFlash,addressRam+subBlock*blockSize, blockSize)) 
					fprintf(stderr," verify OK.");
					else {
						fprintf(stderr,"Verification failed at address 0x%08X.\n",addressFlash);
						return false;
					}
				}
			}
		}
		block += blocksAtOnce;
	}
	fprintf(stderr,"\n");

	return true;
}

/** 
 *  @param fast erase sectors in one call, if not all blank. Otherwise erase sector for sector.
 * code read protection
 */
bool lpcProgram(int fd, LpcFamilyMember const *member, unsigned address, Fifo *data, const LpcProgramConfig *conf) {
	// member.familyMemoryMap->blockSizes[0] is used as alignment
	// find the larges transfer size
//	const int ramLowerSpace = 0x200;	// bytes below are used by ISP handler.
//	const int ramUpperSpace = 0x100 + 32;	// IAP handler and ISP stack requirements (LPC17xx)
	const int blockLimit = member->sizeRamKs[0]*1024 - conf->ramLowerSpace - conf->ramUpperSpace;
	const LpcFamilyMemoryMap *map = member->familyMemoryMap;
	const int addressRam = map->addressRams[0] + conf->ramLowerSpace;
	fprintf(stderr,"Transfer RAM: %dkiB+%dB @ 0x%08X\n",blockLimit/1024,blockLimit%1024,addressRam);

	const int imageSize = fifoCanRead(data);

	// optimized for big images => maximum block with 'Copy RAM to FLASH' unless image is really small.
	int blockSize = 0;
	for (int i=0; i<ELEMENTS(map->blockSizes) && map->blockSizes[i]>0; ++i) {
		if (map->blockSizes[i] <= blockLimit
		&& blockSize<imageSize) 	blockSize = map->blockSizes[i];
	}
	// pad fifo for this block size
	while (fifoCanRead(data) % blockSize) if (!fifoPrintChar(data,0xFF)) return false;

	const int blocks = fifoCanRead(data) / blockSize;
	const int blocksAtOnce = blockLimit / blockSize;	// the will be 1 at least

	fprintf(stderr,"Transmission RAM: %dblocks (%dkiB), units of %dB\n",
		blocksAtOnce, blocksAtOnce*blockSize/1024, blockSize);
	fflush(stderr);

	fprintf(stderr,"Flash address range 0x%08x..0x%08x, ",address,address+blockSize*blocks-1);
	int sectorFrom, sectorTo;
	if (lpcAddressRangeToSectorRange(member,&sectorFrom,&sectorTo,address,address+blockSize*blocks-1)) {
		fprintf(stderr,"sectors: %d..%d\n",sectorFrom,sectorTo);
	}
	else {
		fprintf(stderr,"image does not fit into FLASH\n");
		return false;
	}

	if (!lpcUnlock(fd)) {
		fprintf(stderr,"Failed to unlock device.\n");
		return false;
	}

	if (!lpcBlankCheck(fd,sectorFrom,sectorTo)) {	// erasing required
		if (conf->fast) {
			if (lpcPrepareForWrite(fd,sectorFrom,sectorTo)
			&& lpcErase(fd,sectorFrom,sectorTo)) fprintf(stderr,"Erased en bloc.\n");
			else {
				fprintf(stderr,"Failed to erase sectors %d to %d\n",sectorFrom,sectorTo);
				return false;
			}
		}
		else for (int s=sectorFrom; s<=sectorTo; ++s) {
			if (lpcBlankCheck(fd,s,s)
			|| lpcPrepareForWrite(fd,s,s) && lpcErase(fd,s,s)) ;	// fine
			else {
				fprintf(stderr,"Failed to erase sector %d\n",s);
				return false;
			}
		}
	}

	for (int block=0; block<blocks; ) {
		// transfer bytes
		for (int subBlock=0; subBlock<blocksAtOnce && block+subBlock<blocks; ++subBlock) {
			fprintf(stderr,"\nTransfer block %d to RAM ",block+subBlock);
			if (lpcWriteN(fd,member->ispData,addressRam+subBlock*blockSize,data,blockSize)) {	// fine
				fprintf(stderr,"OK.");
			}
			else {
				fprintf(stderr,"FAILED!\n");
				return false;
			}
		}
		// do flash programming
		for (int subBlock=0; subBlock<blocksAtOnce && block+subBlock<blocks; ++subBlock) {
			const Uint32 addressFlash = address+(block+subBlock)*blockSize;
			const int sector = lpcAddressToSector(member,addressFlash);
			fprintf(stderr,"\nTransfer block %d to FLASH, sector %d ",block+subBlock,sector);
			if (lpcPrepareForWrite(fd,sector,sector)
			&& lpcCopyRamToFlash(fd, addressFlash, addressRam+subBlock*blockSize, blockSize)); // fine
			else {
				fprintf(stderr,"Failed to Copy RAM to FLASH.\n");
				return false;
			}
			if (conf->verify) {
				if (addressFlash==0) {
					fprintf(stderr," verify not possible for sector 0.");
				}
				else {
					if (lpcCompare(fd,addressFlash,addressRam+subBlock*blockSize, blockSize)) 
					fprintf(stderr," verify OK.");
					else {
						fprintf(stderr,"Verification failed at address 0x%08X.\n",addressFlash);
						return false;
					}
				}
			}
		}
		block += blocksAtOnce;
	}
	fprintf(stderr,"\n");

	return true;
}

static inline int min32(int a, int b) {
	return a<b ? a : b;
}

void explicit(const char *msg, bool success) {
	fprintf(stderr,"%s: %s\n",msg,success ? "OK." : "FAILED!");
	if (!success) exit(1);
}

void putChar(char c) {
	if (c>=32 || c=='\n') fprintf(stderr,"%c",c);
	else fprintf(stderr,"\\x%02X",c);
	fflush(stderr);
}

/** Re-writes a fifo, so that the LPCxxxx checksum (vector 5, offset 0x14) is 0.
 */
bool fifoLpcChecksum(Fifo *fifo) {
	Fifo clone = *fifo;
	Fifo writer;
	fifoInitRewrite(&writer,fifo);

	Uint32 checksum = 0;
	for (int i=0; i<7; ++i) {
		Uint32 vector;
		if (!fifoGetUint32(&clone,&vector)) return false;
		checksum += vector;
	}
	fifoSkipWrite(&writer,4*5);
	return fifoPutInt32(&writer,-checksum);
}

// only correct for segment 0
bool lpcChecksumFix(const HexImage *hexImage) {
	Fifo fifo;
	fifoInitRead(&fifo, hexImage->segments[0].data, hexImage->segments[0].size);
	if (hexImage->segments[0].address==0) return fifoLpcChecksum(&fifo);
	else return true;
}

static unsigned address = 0;
static Int32 baud = 230400;
static Int32 crystalHz = 14748000;	// IRC + PLL of LPC17 and LPC23
static Int32 addressCrp = 0x2FC;	// code read protection word address
static Fifo fifoDevice = {};
static int eraseSymbol = -1;
static Int32 flashSizeOverride = -1;	// flash size override
static int manualSymbol = -1;
static Int32 levelCrpSet = 0;		// set this level of protection
static Int32 levelCrpMax = 0;		// limit the level of protection
static Int32 memorySizeOverride = -1;	// RAM 0 size override
static bool quick = false;		// prefer speed to flash lifetime
static Int32 readN = -1;
static bool showInfo = false;
static Int32 timeAfterResetMs = 200;
static Int32 timeoutSerialMs = 1000;	// 500 is enough for LPC17, but not the LPC21s
static bool verify = false;
static Int32 writeN = -1;		// write a specified number of bytes
static bool writeAll = false;		// write all
static Int32 executeAddress = -1;
static int executeSymbol = -1;
static Int32 ucNumber = -1;		// force specific device
static int ucSymbol = -1;		// force user supplied.
static Fifo fifoF = {};
static Fifo fifoM = {};
static Int32 ispProtocolOverride = -1;	// binary / uuencode override
static PairInt32 erase = { 0, -1 };
static Int32 ispReserved[2] = { 0x200, 0x120 };	// RAM used by ISP handler, bottom/top of RAM0
static Int32 userId = -1;
static Int32 userNumber = -1;
static Int32 listB[LPC_BLOCK_SIZES] = { };

static const FifoPoptBool optionBools[] = {
	{ 'i',&showInfo },
	{ 'w',&writeAll },
	{ 'v',&verify },
	{ 'q',&quick },
	{ 'u',&useEcho },
	{}
};

static const FifoPoptInt32 optionInts[] = {
	{ 'a',(Int32*)&address, &fifoParseIntEng },
	{ 'b',&baud, },
	{ 'c',&crystalHz, &fifoParseIntEng },
	{ 'C',&addressCrp, },
	{ 'f',&flashSizeOverride, &fifoParseIntEng },
	{ 'I',&userId, },
	{ 'N',&userNumber, },
	{ 'l',&levelCrpSet, &fifoParseInt },
	{ 'L',&levelCrpMax, &fifoParseInt },
	{ 'm',&memorySizeOverride, &fifoParseIntEng },
	{ 'r',&readN, &fifoParseIntEng },
	{ 't',&timeAfterResetMs, },
	{ 'T',&timeoutSerialMs, },
	{ 'U',&ucNumber, },
	{ 'x',&executeAddress, },
	{}
};

static const char* eraseSymbols[] = {
	"all",
	0
};

static const char* executeSymbols[] = {
	"LMA","LMA+1",
	0
};

static const char* ucSymbols[] = {
	"user",
	0
};

static const char* manualSymbols[] = {
	"contents",
	"numeric",
	"new-devices",
	"crp",
	0
};

static const char* ispProtocols[] = {
	"UUENCODE","BINARY",		// these positions must match the enum im lpcMemories.h !
	0
};

static const FifoPoptSymbol optionSymbols[] = {
	{ 'e',&eraseSymbol, eraseSymbols },
	{ 'h',&manualSymbol, manualSymbols },
	{ 'x',&executeSymbol, executeSymbols },
	{ 'U',&ucSymbol, ucSymbols },
	{ 'P',&ispProtocolOverride, ispProtocols },
	{}
};

static const FifoPoptString optionStrings[] = {
	{ 'd', &fifoDevice },
	{ 'F', &fifoF },
	{ 'M', &fifoM },
	{}
};

static const FifoPoptInt32Range optionInt32Ranges[] = {
	{ 'e', &erase },
	{}
};

static const FifoPoptInt32Tuple optionInt32Tuples[] = {
	{ 'I', 2, ispReserved, ",", },
	{ 'B', 4, listB, ",", &fifoParseIntEng },
	{}
};

// complex parsing section: options -F and -M: lists of pairs

// end of option validator
bool eoo(Fifo *fifo) {
	return 0==fifoCanRead(fifo);
}

const FifoParsePairInt parseSizeXCount = {
	.parseIntA = &fifoParseIntEng,
	.separator = "x",
	.parseIntB = &fifoParseIntCStyle,
	// no validator!
};

const FifoParsePairInt parseSizeAtAddress = {
	.parseIntA = &fifoParseIntEng,
	.separator = "@",
	.parseIntB = &fifoParseIntCStyle,
	// no validator!
};

const FifoParseListPairInt parseF = {
	.parsePair = &parseSizeXCount,
	.separator = ",",
	.validator = &eoo
};

const FifoParseListPairInt parseM = {
	.parsePair = &parseSizeAtAddress,
	.separator = ",",
	.validator = &eoo
};

PairInt listF[3];
PairInt listM[3];

static void showHelp(void) {
	fprintf(stderr,"%s",
	"isp " ISP_VERSION_COPYRIGHT "\n"
	"A command-line ISP programmer for NXP LPC1xxx,2xxx devices.\n\n"
	"Usage: isp <options> [<inputFile>]\n"
	"Options:\n"
	"  -? or --help              : display this help screen.\n"
	"  -a <destinationAddress>   : image destination address (LMA) (sector boundary!!) [0]\n"
	"  -b <baud rate>            : communication baud rate [230400]\n"
	"  -c <f crystal/Hz>         : crystal frequency [14748k]\n"
	"  -d <device>               : serial device [/dev/ttyUSB0]\n"
	"  -e {<sect>[..<sect>]|all} : erase FLASH sectors from..to, or all\n"
	"  -f <flashSize>            : FLASH size override\n"
	"  -h <topic>                : display manual <topic>, start with \"contents\"\n"
	"  -i                        : print member info\n"
	"  -l <CRP level>            : lock code. Set code read protection (CRP) level.\n"
	"  -m <RAM size>             : RAM 0 size override\n"
	"  -o <output file>          : set output file\n"
	"  -p                        : pipe input/output after executing program\n"
	"  -q                        : quick - speed up program at all cost\n"
	"  -r <image size>           : read <image size> bytes from RAM/FLASH\n"
	"  -s                        : show all compiled-in devices.\n"
	"  -t <time/ms>              : time isp waits after RESET deassertion [200]\n"
	"  -T <time/ms>              : timeout of serial communication, step: 100ms [1000]\n"
	"  -u                        : use echo on (bug fix for LPC812)\n"
	"  -v                        : verify FLASH contents\n"
	"  -w                        : write to FLASH\n"
	"  -x {<address>|LMA|LMA+1}  : execute. ARM/THUMB chosen from address LSB\n"
	"  -P {UUENCODE|BINARY}      : data encoding protocol\n"
	"  -R <bottom>,<top>         : set RAM0 spaces reserved for ISP [0x200,0x120]\n"
	"  -D <device database file> : load device descriptions from file.\n"
	"  -I <uC ID>                : set user-defined id\n"
	"  -N <uC number>            : set user-defined number\n"
	"  -C <CRP address>          : address of the 4-byte CRP code [0x2FC]\n"
	"  -L <CRP level>            : enable CRP level (dangerous especially for level 3).\n"
	"  -F 4kix16,32kix14         : define FLASH sector layout to be 16x4kiB, then 14x32kiB\n"
	"  -M 8ki@0x40000000,8ki@... : define uC RAM to be 8kiB at address 0x40000000 and ...\n"
	"  -B 256,512,1024,4096      : define copy-ram-to-flash block sizes to be 256,512... \n"
	"  -U {<uC-number>|user}     : force uC like 2101, do not use part ID.\n\n"
	"Providing an input file on the command line causes implicit option -w\n"
	"If your controller does not work correctly with isp, try low baud rate, low serial\n"
	"timeout (-T 1000 or more) and correct crystal frequency.\n"
	"Try -h contents for online manual\n"
	"\n"
	"Please report bugs to marc@windscooting.com\n"
	);
}

void manualContents(void) {
	fprintf(stderr,
	"The following topics are available from the online manual:\n"
	"  -h numeric                : understanding isp's advanced command-line parsing\n"
	"  -h new-devices            : how to configure isp for new devices\n"
	"  -h crp                    : code read protection (CRP)\n"
	);
}

void manualNumericArguments(void) {
	fprintf(stderr,
	"In the author's opinion, graphic user interfaces may be good thing for beginners\n"
	"but are always a nuisance for the advanced (scripting) user. However, we haven't\n"
	"seen much progress in command-line interfaces for years. The author hopes, that\n"
	"isp's advanced command line features will inspire other programmers to make\n"
	"their (command line) programs more user-friendly. isp uses single char options\n"
	"that accept arguments that are either structured (lists, tuples) or have\n"
	"numeric extensions. Integers can be decimal, 0xhexadecimal, sometimes accept\n"
	"suffixes like k (*1000) or ki (*1024) which are suitable to define common\n"
	"memory sizes. A few examples:\n"
	"  -c 14746k        means: 14,746,000 (Hz) crystal frequency.\n"
	"  -x 0x40000000    means: start program at 0x4000 0000, which is the same as\n"
	"  -x 1Gi           (1GiB above 0).\n"
	"  -e 1             erases sector 1 (the range 1 to 1 of sectors).\n"
	"  -e 1..3          erases sectors 1 to 3.\n"
	"  -e all           erases all sectors of the device in use.\n"
	"  -B 256,512,1024,4096  defines the possible transfer sizes used to copy RAM\n"
	"                   to flash for most (all?) LPCs. It is a 4..5-tuple.\n"
	"Defining the flash sector sizes is done with a list of pairs, with a bit of\n"
	"eye-candy added, as an example LPC213x:\n"
	"  -F 4kix8,32kix14,4kix5      as the short form of\n"
	"  -F 4096x8,32768x14,4096x5\n"
	"Please note the difference between 4k (4000) and 4ki (4096).\n"
	"Also, don't expect every integer to have all formats allowed.\n"
	);
}

void manualNewDevices(void) {
	fprintf(stderr,
	"If any of the options -F, -M, -B, -N is provided, then ALL of them must be provided.\n"
	"This defines a device. -I should be provided to enable automatic selection.\n"
	"Simply put: these option allow you to extend the compiled in table to your needs.\n"
	"In the author's opinion, you should not need more than a datasheet of a new device\n"
	"to program it. No recompilation required. isp uses RAM of your controller, that's\n"
	"why you have to define RAM location and size, too.\n"
	"Option -U user forces use of user-defined layout as does -N xxxx -U xxxx.\n"
	"Normally isp reads the controller's device ID (part ID) and searches for a matching\n"
	"entry from the command line or from the compiled-in table. You have the option to\n"
	"specify an explicit device number (-U 2102 for a LPC2102) to make a hard selection\n"
	"in cases where the part ID is not useful or not wanted. Specifying -U user will\n"
	"always select your memory specification on the command line. Two convenience\n"
	"options are provided to specify total FLASH size (-f) and RAM0 size (-m)\n"
	"Example: you want to program LPC2102. This controller has the same ID as LPC2103,\n"
	"yet the memory sizes are different.\n"
	"Option 1: define LPC2102 from scratch:\n"
	"  -F 4kix4 -M 4ki@1Gi -B 256,512,1024,4096 -N 2102 -U user\n"
	"Option 2: use compiled-in table, which yields LPC2103 and limit the memories:\n"
	"  -f 16ki -m 4ki\n"
	"Option 3: use compiled-in table, specify device number:\n"
	"  -U 2102\n"
	);
}

void manualCrp(void) {
	fprintf(stderr,
	"isp takes care of CRP in the following sense: avoid unintended CRP and add CRP\n"
	"where needed. Achieving CRP3 is intentionally a bit hard to achieve.\n"
	"isp allows upgrading and downgrading of CRP levels 0..2.\n"
	"CRP levels are encoded at memory location 0x2FC (4 bytes). The following rules apply:\n"
	"  a.CRP3 will only be executed if -l 3, -L 3 are given and the image contains\n"
	"    the pattern 0x43218765 at 0x2FC.\n"
	"  b.If 0x2FC contains a CRP pattern and CRP is not enabled (-L) on the command\n"
	"    line an error is issued.\n"
	"  c.If 0x2FC does not contain -1 or 0x12345678 CRP levels 1,2 cannot be applied.\n"
	"Rule a. inhibits unintentional CRP3 if the program does not provide IAP.\n"
	"Rule b. inhibits accidental CRP by the compiler/linker.\n"
	"Rule c. inhibits unwanted executable modification by isp.\n"
	"To unlock a CRP-enabled image use -l 0.\n"
	);
}

bool fifoLpcCrp(Fifo *image, Uint32 addressCrp, Uint32 address, int crpMax, int crp) {
	if (addressCrp&3 || address&3) {
		fprintf(stderr,"Alignment error of destination address or CRP bytes.\n");
		return false;
	}
	if (crpMax>3 || crpMax<0) {
		fprintf(stderr,"Maximum CRP level (-L) out of range.\n");
		return false;
	}
	if (crp>3 || crp<0) {
		fprintf(stderr,"CRP level (-l) out of range.\n");
		return false;
	}
	if (crp>crpMax) {
		fprintf(stderr,"CRP level (-l) not allowed.\n");
		return false;
	}
	if (addressCrp<address) {
		if (crp>0) {
			fprintf(stderr,"Cannot provide CRP: destination address too high.\n");
			return false;
		}
		else return true; 	// nothing to do
	}

	Fifo clone = *image;
	if (fifoCanRead(&clone)>addressCrp-address) {
		fifoSkipRead(&clone,addressCrp-address);
		Fifo writer;
		fifoInitRewrite(&writer,&clone);
		Uint32 crpWord = 0;
		if (!fifoGetUint32(&clone,&crpWord)) return false;
		const int crpExe = lpcCrpCodeToLevel(crpWord);
		if (crpExe>crpMax) {
			fprintf(stderr,"Warning: image has CRP (level %d)\n",crpExe);
		}
		if ((crp==3 || crpMax==3 || crpExe==3)		// attempt for CRP3
		&& !(crp==3 && crpMax==3 && crpExe==3)) {	// requirement for CRP3 
			fprintf(stderr,"CRP level 3 needs -l3 -L3 and image to contain 0x43218765@0x2FC.\n");
			return false;
		}
		if (crp==-1 && crpExe>crpMax) {
			fprintf(stderr,"Error: image CRP (level %d) too high\n",crpExe);
			return false;
		}
		// setting crp level
		if (crp!=-1 && crpExe!=crp) {
			if (crpWord==~0u	// erased memory pattern
			|| crpExe>0) {		// any CRP level
				fprintf(stderr,"Warning: image location 0x%04X (CRP) modified.\n",addressCrp);
				fifoPutInt32(&writer,lpcCrpLevelToCode(crp));
			}
			else {
				fprintf(stderr,	"Warning: image location 0x%04X (CRP) must be blank (-1)"
						" or CRP level 1..3\n",addressCrp);
				return false;
			}
		}
		return true;
	}
	else {
		fprintf(stderr,"Info: image too small for CRP.\n");
		return true;
	}
}

bool lpcCrp(const HexImage *hexImage, Uint32 addressCrp, int crpMax, int crp) {
	Fifo fifo;
	fifoInitRead(&fifo,(char*)hexImage->segments[0].data, hexImage->segments[0].size);
	return fifoLpcCrp(&fifo,addressCrp,hexImage->segments[0].address,crpMax,crp);
}

int noConnection(void) {
	fprintf(stderr,"No connection/connection lost (try different baud rate/crystal?).\n");
	return 1;
}

/*
Planned features:
 - baud probing: -b max/high/low
 - device detection/bootcode
 - command-line sector map
 - load at address
 - read back
 - serial number logging of programmed device.
 - piping
*/
int main(int argc, const char **argv) {
	struct timespec ts;
	//clock_getres(CLOCK_PROCESS_CPU_TIME_ID,&ts);
	clock_getres(CLOCK_REALTIME,&ts);
	fprintf(stderr,"Clock resolution: %lds %ldns\n",ts.tv_sec,ts.tv_nsec);

	char lineBuffer[1024];
	Fifo fifoCmdLine = { lineBuffer, sizeof lineBuffer };
	Fifo fifoNonOptions = { lineBuffer, sizeof lineBuffer };

	if (!fifoPoptAccumulateCommandLine(&fifoCmdLine,argc,argv)) {
		fprintf(stderr,"Command line too long.\n");
		return 1;
	}

	if (fifoPoptScanOption(&fifoCmdLine,'?',"help")) {
		showHelp();
		return 0;
	}

	while (fifoCanRead(&fifoCmdLine)) {
		if (false
		|| fifoPoptBool(&fifoCmdLine,optionBools)
		|| fifoPoptInt32(&fifoCmdLine,optionInts)
		|| fifoPoptInt32Range(&fifoCmdLine,optionInt32Ranges)
		|| fifoPoptInt32Tuple(&fifoCmdLine,optionInt32Tuples)
		|| fifoPoptString(&fifoCmdLine,optionStrings)
		|| fifoPoptSymbol(&fifoCmdLine,optionSymbols)
		|| fifoPoptNonOptionAccumulate(&fifoCmdLine,&fifoNonOptions)
		) {
			// fine
		}
		else {
			fprintf(stderr,	"Invalid option/parameter: \"%s\"\n",fifoReadPositionToString(&fifoCmdLine));
			fprintf(stderr,	"Try \"-?\" for help\n"
					"Try \"-h contents\" for manual overview\n" 
			);
			return 1;
		}
	}

	if (manualSymbol>-1) {
		switch(manualSymbol) {
			case 0: manualContents(); return 0;
			case 1: manualNumericArguments(); return 0;
			case 2: manualNewDevices(); return 0;
			case 3: manualCrp(); return 0;
			default: fprintf(stderr,"Manual error\n"); return 1;
		}
	}
	// early parsing
	int nFs = 0;
	int nMs = 0;
	if (fifoCanRead(&fifoM)) {
		if (1<=(nMs=fifoParseListPairInt(&fifoM,&parseM,listM,ELEMENTS(listM)))) {
			fprintf(stderr,"Explicit memory definition: %dkiB RAM at 0x%08X.\n",listM[0].a/1024,listM[0].b);
		}
		else {
			fprintf(stderr,"Error parsing explicit memory definition.\n");
			return 1;
		}
	}
	if (fifoCanRead(&fifoF)) {
		if (1<=(nFs=fifoParseListPairInt(&fifoF,&parseF,listF,ELEMENTS(listF)))) {
			fprintf(stderr,"Explicit sector layout definition:");
			for (int i=0; i<nFs; ++i) {
				if (i!=0) fprintf(stderr,", ");
				fprintf(stderr,"%d sectors of %dkiB",listF[i].b,listF[i].a/1024);
			}
			fprintf(stderr,"\n");
		}
		else {
			fprintf(stderr,"Error parsing explicit sector layout definition.\n");
			return 1;
		}
	}

	// Explicit file on command line is intended for writing as default.
	const char *inputFile = fifoCanRead(&fifoNonOptions) ? fifoReadPositionToString(&fifoNonOptions) : 0;
	if (inputFile) writeAll = true;

	const char *device = fifoCanRead(&fifoDevice) ? fifoReadPositionToString(&fifoDevice) : "/dev/ttyUSB0";
	fprintf(stderr,"Communication device=%s, speed=%dbd, uC crystal=%dkHz\n",device,baud,crystalHz/1000);
	int fd = serialOpenBlockingTimeout(device,baud,timeoutSerialMs/100);

	lpcReset(fd,true);
	lpcBsl(fd,true);
	usleep(10*1000);
	lpcReset(fd,false);
	usleep(timeAfterResetMs*1000);	// DS1818 resets for at least 150ms
	lpcBsl(fd,false);

	int partId = 0;
	int bootCodeVersion = 0;
	fprintf(stderr,"Sync:"); fflush(stderr);
	if (lpcSync(fd,crystalHz)) {
		fprintf(stderr,"OK.");
		fflush(stderr);
	}
	else return noConnection();

	if (!useEcho) {
		fprintf(stderr,", echo off:"); fflush(stderr);
		if (lpcEcho(fd,false)) {
			fprintf(stderr,"OK.");
			fflush(stderr);
		}
		else return noConnection();
	}

	fprintf(stderr,", baud:"); fflush(stderr);
	if (lpcBaud(fd,baud,1)) {
		fprintf(stderr,"OK.");
		fflush(stderr);
	}
	else return noConnection();

	fprintf(stderr,", boot code:" ); fflush(stderr);
	if (lpcReadBootCodeVersion(fd,&bootCodeVersion)) {
		fprintf(stderr,"%d.%d.",bootCodeVersion>>8,bootCodeVersion&0xFF);
		fflush(stderr);
	}
	else return noConnection();

	fprintf(stderr,", device Id:"); fflush(stderr);
	if (lpcReadPartId(fd,&partId)) {	// fine
		fprintf(stderr,"0x%08X\n",partId);
	}
	else return noConnection();

	// calculate device
	// user (command-line) defined device...
	LpcFamilyMemoryMap familyMemoryMapUser = {};
	LpcFamilyMember familyMemberUser = {
		.familyMemoryMap = &familyMemoryMapUser
	};

	//bool userDefinedAvailable = false;

	if (nFs>=1		// option -F
	|| nMs>=1		// option -M
	|| listB[0]!=0		// option -B
	|| userNumber!=-1	// option -N
	|| userId!=-1		// option -I
	) if (nFs>=1 && nMs>=1 && listB[0]!=0 && userNumber!=-1) {
		if (nFs<1) {
			fprintf(stderr,"Invalid FLASH layout definition.\n");
			return 1;
		}
		familyMemoryMapUser.addressFlash = 0;
		for (int ss=0; ss<nFs; ++ss) {
			familyMemoryMapUser.sectorArrays[ss].sizeK = listF[ss].a / 1024;
			familyMemoryMapUser.sectorArrays[ss].n = listF[ss].b;
		}

		if (nMs<1) {
			fprintf(stderr,"Invalid RAM layout definition.\n");
			return 1;
		}
		for (int ms=0; ms<nMs; ++ms) {
			familyMemberUser.sizeRamKs[ms] = listM[ms].a / 1024;
			familyMemoryMapUser.addressRams[ms] = listM[ms].b;
		}

		for (int b=0; b<ELEMENTS(familyMemoryMapUser.blockSizes); ++b)
			familyMemoryMapUser.blockSizes[b] = listB[b];

		familyMemberUser.sizeFlashK = lpcFamilySectorOffset(&familyMemoryMapUser,
			lpcFamilySectorHighest(&familyMemoryMapUser)+1) / 1024;
		familyMemberUser.number = userNumber;
		familyMemberUser.id = userId;		// -1 hopefully matches no real device. 0 neither.
		//userDefinedAvailable = true;
	}
	else {
		fprintf(stderr,"Options -F, -M, -B, -N are required for defining new device.\n");
		return 1;
	}

	// select device: user/probed by id/number
	// the user-defined device is always checked first, to be able to override table entries
	LpcFamilyMember member = {};
	LpcFamilyMemoryMap memoryMap = {};
	bool deviceDetected = true;
	if (ucSymbol!=-1 && ucNumber!=-1) {
		fprintf(stderr,"Competing options -U.\n");
		return 1;
	}
	if (ucSymbol==0			// 'user' selected
	|| ucNumber==familyMemberUser.number	// selected by number defined on command-line
	) {
		member = familyMemberUser;
		memoryMap = familyMemoryMapUser;
		deviceDetected = false;
	}
	else if (ucNumber!=-1) {
		LpcFamilyMember const *tMember = lpcFindByNumber(lpcFamilyMembersXxxx,ucNumber);
		if (tMember!=0) {
			member = *tMember;
			memoryMap = *tMember->familyMemoryMap;
			deviceDetected = false;
		}
		else {
			fprintf(stderr,"Cannot find LPC%d\n",ucNumber);
			return 1;
		}
	}
	else {	// find by ID
		if (partId==familyMemberUser.id) {
			member = familyMemberUser;
			memoryMap = familyMemoryMapUser;
		}
		else {
			LpcFamilyMember const *tMember = lpcFindById(lpcFamilyMembersXxxx,partId);
			if (tMember!=0) {
				member = *tMember;
				memoryMap = *tMember->familyMemoryMap;
			}
			else {
				fprintf(stderr,"Cannot find part ID0x%08X",partId);
				return 1;
			}
		}
	}
	member.familyMemoryMap = &memoryMap;	// pointer rearrangement required.
	fprintf(stderr,"%s: LPC%d, FLASH:%dkiB, RAM0:%dkiB, RAM1:%dkiB, RAM2:%dkiB\n",
		deviceDetected ? "Detected" : "Selected",
		member.number,member.sizeFlashK,member.sizeRamKs[0],member.sizeRamKs[1],member.sizeRamKs[2]
	);
	// now patch the device with command line overrides
	if (flashSizeOverride!=-1) member.sizeFlashK = flashSizeOverride/1024;
	if (memorySizeOverride!=-1) member.sizeRamKs[0] = memorySizeOverride/1024;
	if (ispProtocolOverride!=-1) member.ispData = ispProtocolOverride;

	// show info, if desired
	if (showInfo) {
		char infoBuffer[2048];
		Fifo fifoInfo = { infoBuffer, sizeof infoBuffer };
		fifoPrintLpcFamilyMember(&fifoInfo,&member);
		fprintf(stderr,"Device information:\n");
		fflush(stderr);
		ioWriteFifo(2,&fifoInfo);
	}

	// read image if desired
	if (-1!=readN) {
		char imageBuffer[512*1024];
		Fifo fifoImage = { imageBuffer, sizeof imageBuffer };

		if (lpcRead(fd,member.ispData,&fifoImage,address,readN)) {
			if (ioWriteFifo(1,&fifoImage)) {	// write to stdout, fd 1
				fprintf(stderr,"FLASH read successfully.\n");
			}
			else {
				fprintf(stderr,"Broken pipe.\n");
				return 1;
			}
		}
		else {
			fprintf(stderr,"Error reading FLASH.\n");
			return 1;
		}
	}

	// explicit erasure of sectors
	const bool eraseAll = eraseSymbol==0;
	if (eraseAll) {
		erase.a = 0;
		erase.b = lpcAddressToSector(&member, member.familyMemoryMap->addressFlash+member.sizeFlashK*1024 -1);
		if (erase.b==-1) {
			fprintf(stderr,"Failed to calculate last sector\n");
			return 1;
		}
	}

	if (erase.a<=erase.b) {
		if (true
		&& lpcUnlock(fd)
		&& lpcPrepareForWrite(fd,erase.a,erase.b)
		&& lpcErase(fd,erase.a,erase.b)) {
			fprintf(stderr,"Erased sectors %d to %d\n",erase.a,erase.b);
		}
		else {
			fprintf(stderr,"Failed to erase sectors %d to %d\n",erase.a,erase.b);
			fflush(stderr);
			return 1;
		}
	}

	// writing
	if (writeAll || writeN!=-1) {
		if (writeAll && writeN!=-1) {
			fprintf(stderr,"Both write N and write all requested.\n");
			return 1;
		}

		// writeN only limits the maximum size; does not force padding in the first place
		/*
		const int inputFd = inputFile!=0 ? open(inputFile,O_RDONLY) : 0;
		if (inputFd==-1) {
			fprintf(stderr,"Cannot open input file \"%s\"\n",inputFile);
			return 1;
		}
		char c;
		for (int b=0; (writeN==-1 || b<writeN) && (fifoCanWrite(&fifoImage) && 1==read(inputFd,&c,1)); b++) {
			fifoWrite(&fifoImage,c);
		}
		close(inputFd);
		*/


		Uint8 hexImageBuffer[512*1024];
		HexImage hexImage = { hexImageBuffer, sizeof hexImageBuffer, };
		hexImageInit(&hexImage);
		if (!hexFileLoad(inputFile,&hexImage)) {
			fprintf(stderr,"Could not load file \"%s\".\n",inputFile ? inputFile : "<stdin>");
			return 1;
		}
		if (hexImageSegments(&hexImage)!=1) {
			fprintf(stderr,"Error: %d file segments in image, but only 1 supported.\n",
				hexImageSegments(&hexImage));
			return 1;
		}
		if (address!=0) hexImage.segments[0].address = address;	// manual override

		// handle checksum in image vector table entry #7
		if (lpcChecksumFix(&hexImage)) {
			const int n = hexImage.segments[0].size;
			fprintf(stderr,"Image: %d = 0x%06X Bytes\n", n,n);
		}
		else {
			fprintf(stderr,"Image too small for (required) checksum calculation.\n");
			return 1;
		}

		// handle code locking
		//if (!fifoLpcCrp(&fifoImage,addressCrp,address,levelCrpMax,levelCrpSet)) {
		if (!lpcCrp(&hexImage,addressCrp,levelCrpMax,levelCrpSet)) {
			fprintf(stderr,"CRP problem.\n");
			return 1;
		}

		const LpcProgramConfig lpcProgramConfig = {
			.verify = verify,
			.fast = quick,
			.ramLowerSpace = ispReserved[0],
			.ramUpperSpace = ispReserved[1]
		};
		if (lpcUnlock(fd) && lpcProgramNew(fd,&member,&hexImage,&lpcProgramConfig)) ;	// fine
		else {
			fprintf(stderr,"Error programming FLASH.\n");
			return 1;
		}
	}

	// program execution
	if (executeAddress!=-1) {
		if (lpcUnlock(fd) && lpcGo(fd,executeAddress&~1,executeAddress&1)) {
			// no return from here!
			fprintf(stderr,"Started program at address 0x%08X\n",executeAddress);
		}
		else {
			fprintf(stderr,"Failed to execute.\n");
			return 1;
		}
	}
	// restore settings
	//tcsetattr(fd,TCSANOW,&terminalSettingsSaved);
	close(fd);

	return 0;
}
