/*
  ramloader.c - main program of ramloader, a program loader (into RAM) for NXP LPC ARM controllers.
  Copyright 2011-2013 Marc Prager
 
  ramloader is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  ramloader is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with ramloader.
  If not see <http://www.gnu.org/licenses/>
 */

#include <fifo.h>
#include <fifoParse.h>
#include <c-linux/fd.h>
#include <c-linux/hexFile.h>
#include <stdlib.h>
#include <macros.h>

static char fifoInBuffer[1024]; static Fifo _fifoIn = { fifoInBuffer, sizeof fifoInBuffer, };
static char fifoOutBuffer[4*1024]; static Fifo _fifoOut = { fifoOutBuffer, sizeof fifoOutBuffer, };
static char fifoScanBuffer[1024]; static Fifo _fifoScan = { fifoScanBuffer, sizeof fifoScanBuffer, };
static char fifoUcInBuffer[1024]; static Fifo _fifoUcIn = { fifoUcInBuffer, sizeof fifoUcInBuffer, };
static char fifoUcOutBuffer[1024]; static Fifo _fifoUcOut = { fifoUcOutBuffer, sizeof fifoUcOutBuffer };
static int fdSerial = -1;

Fifo
	*fifoIn = &_fifoIn,		// Console -> prog
	*fifoOut = &_fifoOut,		// prog -> Console
	*fifoErr = &_fifoOut;		// prog -> Console
static Fifo
	*fifoScan = &_fifoScan,		// uC -> prog
	*fifoUcIn = &_fifoUcIn,		// uC -> prog
	*fifoUcOut = &_fifoUcOut;	// prog -> uC

#include <fifoPrint.h>

bool fifoPrintCharEscaped2(Fifo *fifo, bool onlyAscii, char c) {
	if (0x20<=c && c<=127
	|| c<0 && !onlyAscii	// extended char set
	|| c=='\n' || c=='\r') return fifoPrintChar(fifo,c);
	else return fifoPrintString(fifo,"\\x") && fifoPrintHex(fifo,(int)c & 0xFF,2,2);
}

bool fifoPrintCharAnsiEscaped(Fifo *fifo, bool disableAnsi, bool onlyAscii, char c) {
	if (c=='\x1B' && !disableAnsi) return fifoPrintChar(fifo,c);
	else return fifoPrintCharEscaped2(fifo,onlyAscii,c);
}

void doIo(void) {
	// uC -> prog
	if (fifoCanWrite(fifoUcIn) && fdCanRead(fdSerial)) fifoWrite(fifoUcIn,fdRead(fdSerial));

	// prog -> uC
	while (fifoCanRead(fifoUcOut) && fdCanWrite(fdSerial)) fdWrite(fdSerial,fifoRead(fifoUcOut));

	// prog -> console
	while (fifoCanRead(fifoOut) && fdCanWrite(1)) fdWrite(1,fifoRead(fifoOut));


	// console -> prog
	if (fifoCanWrite(fifoIn) && fdCanRead(0)) {
		const int c = fdRead(0);
		if (c>=0) fifoWrite(fifoIn,(char)c);
		else exit(0);		// EOF program termination
	}
}

void flushOut(void) {
	while (fifoCanRead(fifoOut)) fdWrite(1,fifoRead(fifoOut));
}

bool sendCommand(const unsigned char *cmd, int n) {
	if (n<=fifoCanWrite(fifoUcOut)) {
		fifoWriteN(fifoUcOut,cmd,n);
		while (fifoCanRead(fifoUcOut)) doIo();
		return true;
	}
	else return false;
}

bool receiveCommand(unsigned char *cmd, int n) {
	while (fifoCanRead(fifoUcIn) < n) doIo();
	fifoReadN(fifoUcIn,cmd,n);
	return true;
}

#include <fifoPrint.h>

#define FORCE(x) if (!(x)) {\
	fprintf(stderr,"Program error, failed condition: %s\n",#x);\
	exit(1);\
	} else ;

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <c-linux/serial.h>

const char* ramloader = "ramloader";

void ramloaderSync(void) {
	// flush input
	fifoSkipRead(fifoUcIn, fifoCanRead(fifoUcIn));

	unsigned char response = 0;
	const unsigned char test = 0xA0;

	printf("<sync>"); fflush(stdout);
	unsigned char cmdSync[] = { 5, test };
	FORCE(sendCommand(cmdSync, sizeof cmdSync));
	FORCE(receiveCommand(&response, sizeof response));
	FORCE((~response & 0xFF)==test);
}

/** Calculates the checksum of a 256 byte block.
 */
Uint16 checksum(const Uint8 *data) {
	Uint16 sum = 0;
	for (int i=0; i<256; ++i) sum += (Uint16)data[i] & 0xFF;
	return sum;
}

bool downloadCode(const char *fnImage, unsigned address, bool verify, bool debug) {
	Uint8	ram[0x20000];
	HexImage hexImage = { ram, sizeof ram };
	hexImageInit(&hexImage);

	const unsigned char cmdResetAddress = 0x1;
	const unsigned char cmdWrite = 0x2;
	const unsigned char cmdSetAddress = 0x7;	// + 4 bytes absolute address.
	const unsigned char cmdExecute = 0x4;
	const unsigned char cmdDebug = 0x8;		// keep ramloader UART and start program
	fifoPrintString(fifoOut,"<Downloading image>\n");
	ramloaderSync();		// armloader lost sync at this point for a long time.
	if (hexFileLoad(fnImage,&hexImage)) {
		for (int s=0; s<=hexImageSegments(&hexImage); s++) {
			const Segment *segment = &hexImage.segments[s];
			fifoPrintSegment(fifoOut,segment);
			fifoPrintString(fifoOut," :");
			if (segment->size==0) continue;

			if (addressIsValid(segment->address)) {	// send setAddress
				const unsigned char setAddress[] = {
					cmdSetAddress,
					segment->address & 0xFF,
					segment->address>>8 & 0xFF,
					segment->address>>16 & 0xFF,
					segment->address>>24 & 0xFF
				};
				FORCE(sendCommand(setAddress, sizeof setAddress));
				unsigned char response;
				FORCE(receiveCommand(&response,sizeof response));
				FORCE(response=='A');
			}
			else if (s==0) {
				FORCE(sendCommand(&cmdResetAddress, sizeof cmdResetAddress));
				unsigned char response;
				FORCE(receiveCommand(&response,sizeof response));
				FORCE(response==0xA5);
			}
			else {
				fifoPrintString(fifoOut,"Missing segment address.\n");
				return false;
			}

			for (int b=0; b<segment->size; b+=256) {
				Uint8 block[256];
				memset(block,0xFF,sizeof block);
				memcpy(block,segment->data+b,segment->size-b >= 256 ? 256 : segment->size-b);
				FORCE(sendCommand(&cmdWrite, sizeof cmdWrite));
				FORCE(sendCommand(block, sizeof block));
				const Uint16 sum;
				FORCE(receiveCommand((unsigned char*)&sum,sizeof sum));
				fifoPrintChar(fifoOut,'.');
			/*	fifoPrintString(fifoOut,"sum=0x");
				fifoPrintHex(fifoOut,sum,4,4);
				fifoPrintString(fifoOut,", sum(block)=0x");
				fifoPrintHex(fifoOut,checksum(block),4,4);
				fifoPrintLn(fifoOut);
			*/
				flushOut();
				FORCE(sum==checksum(block));
			}
			fifoPrintStringLn(fifoOut,"OK.");
		}
		fifoPrintStringLn(fifoOut,"<Executing code>.");
		if (!debug) {
			FORCE(write(fdSerial,&cmdExecute,1));
		}
		else {
			FORCE(write(fdSerial,&cmdDebug,1));
		}
		return true;
	}
	else {
		fifoPrintString(fifoOut,"Cannot load image.\n");
		return false;
	}
}

int main(int argc, char* argv[]) {
	const char *envTty = "ARM_TTY";
	const char *envDebug = "ARM_DEBUG";
	struct {
		const char *port;
		unsigned baud;
		unsigned address;
		unsigned timeoutMs;
		bool verify;
		bool debug;
		bool disableAnsi;
		bool onlyAscii;
	}
	options = {
		getenv(envTty) ? getenv(envTty) : "/dev/ttyUSB0",
		.baud = 115200,
		.address = 0x40000000,
		.verify = false,		// do not verify by default
		.debug = getenv(envDebug) ? getenv(envDebug)[0]=='1' : false,	// alternate start of program
		.disableAnsi = false,
		.onlyAscii = false,
	};

	for (int optChar; -1!=(optChar = getopt(argc,argv,"Aab:d:t:hgv?")); ) switch(optChar) {

		case 'A':	options.onlyAscii = true; break;
		case 'a':	options.disableAnsi = true; break;
		case 'd':	options.port = optarg; break;
		case 'b':	options.baud = strtol(optarg,0,0); break;
		case 'v':	options.verify = true; break;
		case 't':	options.timeoutMs = strtol(optarg,0,0); break;
		case 'g':	options.debug = true; break;
		case 'h':
		case '?':
			printf("%s 2.1 (formerly known as armloader), (C) Marc Prager, 2005-2012\n",ramloader);
			printf("NXP LPC8xx/1xxx/2xxx/3xxx/4xxx RAM loader program\n");
			printf("usage: %s [options] <image-file>\n",ramloader);
			printf("<image-file> may be binary or intel-hex in case of extension .hex\n");
			printf("options:\n");
			printf("  -a                : disable ANSI escape sequences (0x1B) [off]\n");
			printf("  -b <baud rate>    : serial communication speed [%d]\n",options.baud);
			printf("  -d <device>       : default serial communication device [%s",options.port);
			printf("] optionally set by variable %s\n",envTty);
			printf("  -g                : run program in debug mode [optionally set by variable %s\n",envDebug);
			printf("  -A                : ASCII characters only (<128), everything else as \\xXX\n");
			printf("  -t <timeout/ms>   : serial port receive timeout\n");
			printf("  -v                : verify downloaded image\n");
			printf("  -h or -?          : help\n\n");
			return 1;
		default :
			printf("invalid option char '%c' (\\x%02x) - try help (-?)\n",optChar,optChar);
			return 2;
	}

	if (optind>=argc) {
		fprintf(stderr,"%s: missing input file. Try option: -?\n",ramloader);
		return 1;
	}
	
	const char *fileName = argv[optind];

	//printf("%s:tty=%s\n",ramloader,options.port);
	fdSerial = serialOpenBlockingTimeout(options.port,options.baud,options.timeoutMs/100);
	FORCE(fdSerial>=0);

	const char *pattern = "<RAMLOADER: hardware reset>\n";
	
	// disable /BSL=RTS on Olimex LPC21-boards and AEGMIS-GIM1/2
	// do /RESET=DTR
	serialSetRts(fdSerial,true);
	serialSetDtr(fdSerial,false);
	usleep(200*1000);
	serialSetDtr(fdSerial,true);
	usleep(200*1000);

	(void)fifoScan;
	while (true) {
		doIo();

		if (fifoCanRead(fifoUcIn)
		&& 4<=fifoCanWrite(fifoOut)
		&& fifoCanWrite(fifoScan)) {
			const char c = fifoRead(fifoUcIn);
			fifoWrite(fifoScan,c);
			fifoPrintCharAnsiEscaped(fifoOut,options.disableAnsi,options.onlyAscii,c);
			if (fifoSearch(fifoScan,pattern)) {
				if (!downloadCode(fileName,options.address,options.verify,options.debug)) {
					fifoPrintString(fifoErr,"\nERROR: Program NOT started.\n");
					break;
				}
			}
			// restart scan.
			// reset input
		}

		if (fifoCanRead(fifoIn) &&fifoCanWrite(fifoUcOut)) fifoWrite(fifoUcOut, fifoRead(fifoIn));
		usleep (86);	// otherwise 100% CPU load
	}
	doIo();

	close(fdSerial);
	return 0;
}
