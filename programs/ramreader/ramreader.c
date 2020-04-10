/*
  ramreader.c - inverse to ramloader. See ramloader.c for more details.
  Copyright 2011-2013 Marc Prager
 
  ramreader is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  ramreader is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with ramreader.
  If not see <http://www.gnu.org/licenses/>
 */

#include <fifo.h>
#include <fifoParse.h>
#include <c-linux/fd.h>
#include <c-linux/hexFile.h>
#include <stdlib.h>
#include <macros.h>
#include <int32Math.h>

static char fifoInBuffer[1024]; static Fifo _fifoIn = { fifoInBuffer, sizeof fifoInBuffer, };
static char fifoOutBuffer[1024*1024]; static Fifo _fifoOut = { fifoOutBuffer, sizeof fifoOutBuffer, };
static char fifoErrBuffer[4*1024]; static Fifo _fifoErr = { fifoErrBuffer, sizeof fifoErrBuffer, };
static char fifoScanBuffer[1024]; static Fifo _fifoScan = { fifoScanBuffer, sizeof fifoScanBuffer, };
static char fifoUcInBuffer[1024]; static Fifo _fifoUcIn = { fifoUcInBuffer, sizeof fifoUcInBuffer, };
static char fifoUcOutBuffer[1024]; static Fifo _fifoUcOut = { fifoUcOutBuffer, sizeof fifoUcOutBuffer };
static int fdSerial = -1;

Fifo
	*fifoIn = &_fifoIn,		// Console -> prog
	*fifoOut = &_fifoOut,		// prog -> output (file/stdout)
	*fifoErr = &_fifoErr;		// prog -> Console
static Fifo
	*fifoScan = &_fifoScan,		// uC -> prog
	*fifoUcIn = &_fifoUcIn,		// uC -> prog
	*fifoUcOut = &_fifoUcOut;	// prog -> uC

#include <fifoPrint.h>

bool fifoPrintCharEscaped(Fifo *fifo, char c) {
	if (0x20<=c && c<=127
	|| c=='\n' || c=='\r') return fifoPrintChar(fifo,c);
	else return fifoPrintString(fifo,"\\x") && fifoPrintHex(fifo,(int)c & 0xFF,2,2);
}

void doIo(void) {
	// uC -> prog
	if (fifoCanWrite(fifoUcIn) && fdCanRead(fdSerial)) fifoWrite(fifoUcIn,fdRead(fdSerial));

	// prog -> uC
	while (fifoCanRead(fifoUcOut) && fdCanWrite(fdSerial)) fdWrite(fdSerial,fifoRead(fifoUcOut));

	// prog -> console (stderr)
	while (fifoCanRead(fifoErr) && fdCanWrite(2)) fdWrite(2,fifoRead(fifoErr));

	// prog -> stdout
	while (fifoCanRead(fifoOut) && fdCanWrite(1)) fdWrite(1,fifoRead(fifoOut));
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

const char* ramreader = "ramreader";

void ramreaderSync(void) {
	// flush input
	fifoSkipRead(fifoUcIn, fifoCanRead(fifoUcIn));

	unsigned char response = 0;
	const unsigned char test = 0xA0;

	fprintf(stderr,"<sync>"); fflush(stderr);
	unsigned char cmdSync[] = { 5, test };
	FORCE(sendCommand(cmdSync, sizeof cmdSync));

	FORCE(receiveCommand(&response, sizeof response));

	FORCE((~response & 0xFF)==test);
}

bool readCode(Fifo *data, unsigned n) {
	const unsigned char cmdResetAddress = 0x1;
	const unsigned char cmdWrite = 0x2;
	const unsigned char cmdRead = 0x3;
	const unsigned char cmdSetAddress = 0x7;	// + 4 bytes absolute address.

	fifoPrintString(fifoErr,"<Reading image>\n");
	ramreaderSync();		// armloader has lost sync at this point for a long time.
	FORCE(sendCommand(&cmdResetAddress,sizeof cmdResetAddress));
	unsigned char a5;
	FORCE(receiveCommand(&a5,sizeof a5) && a5==0xA5);	// reset address response
	char buffer[256];
	for (int a=0; a<n; a+=sizeof buffer) {
		FORCE(sendCommand(&cmdRead,sizeof cmdRead));
		FORCE(receiveCommand((unsigned char*)buffer,sizeof buffer));
		const unsigned int nLeft = int32Min(sizeof buffer, n-a);
		FORCE(fifoCanWrite(data)>=nLeft);
		fifoWriteN(data,buffer,nLeft);
		fifoPrintChar(fifoErr,'.');
		doIo(); 
	}
	fifoPrintChar(fifoErr,'\n');
	return true;
}

int main(int argc, char* argv[]) {
	const char *envTty = "ARM_TTY";
	struct {
		const char *port;
		Uint32 baud;
		Uint32 timeResetMs;
		Uint32 timeoutMs;
		Uint32 n;
	}
	options = {
		getenv(envTty) ? getenv(envTty) : "/dev/ttyUSB0",
		115200,
		200,
		1000,
		1024,
	};

	for (char optChar; -1!=(optChar = getopt(argc,argv,"b:d:r:t:T:h?")); ) switch(optChar) {

		case 'b':	options.baud = strtol(optarg,0,0); break;
		case 'd':	options.port = optarg; break;
		case 'r':	options.n = strtol(optarg,0,0); break;
		case 't':	options.timeResetMs = strtol(optarg,0,0); break;
		case 'T':	options.timeoutMs = strtol(optarg,0,0); break;
		case 'h':
		case '?':
		default :
			printf("%s 1.0 (formerly known as armloader), (C) Marc Prager, 2013\n",ramreader);
			printf("NXP LPC1xxx/2xxx RAM reader program\n");
			printf("usage: %s [options] <output-file>\n",ramreader);
			printf("<image-file> will be binary\n");
			printf("options:\n");
			printf("  -b <baud rate>    : serial communication speed [%d]\n",options.baud);
			printf("  -d <device>       : default serial communication device [%s",options.port);
			printf("] optionally set by variable %s\n",envTty);
			printf("  -r <n>            : read <n> bytes [%u]\n",options.n);
			printf("  -t <timeout/ms>   : serial port receive timeout\n");
			printf("  -T <timeout/ms>   : serial port receive timeout\n");
			printf("  -h or -?          : help\n\n");
			return 1;
	}

/*
	if (optind>=argc) {
		fprintf(stderr,"%s: missing output file. Try option: -?\n",ramreader);
		return 1;
	}
*/	
	const char *fileName = argv[optind];

	fdSerial = serialOpenBlockingTimeout(options.port,options.baud,options.timeoutMs/100);
	FORCE(fdSerial>=0);

	const char *pattern = "<RAMLOADER: hardware reset>\n";
	
	// disable /BSL=RTS on Olimex LPC21-boards and AEGMIS-GIM1/2
	// do /RESET=DTR
	serialSetRts(fdSerial,true);
	serialSetDtr(fdSerial,false);
	usleep(options.timeResetMs*1000);
	serialSetDtr(fdSerial,true);

	(void)fifoScan;
	while (true) {
		doIo();

		if (fifoCanRead(fifoUcIn)
		&& 4<=fifoCanWrite(fifoErr)
		&& fifoCanWrite(fifoScan)) {
			const char c = fifoRead(fifoUcIn);
			fifoWrite(fifoScan,c);
			fifoPrintCharEscaped(fifoErr,c);
			if (fifoSearch(fifoScan,pattern)) {
				readCode(fifoOut,options.n);
				while (fifoCanRead(fifoOut)
				|| fifoCanRead(fifoErr)) doIo();
				return 0;
			}
		}

		if (fifoCanRead(fifoIn) &&fifoCanWrite(fifoUcOut)) fifoWrite(fifoUcOut, fifoRead(fifoIn));
	}
	doIo();


	close(fdSerial);
	return 0;
}
