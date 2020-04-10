/*
  hexFile.c
  Copyright 2012 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <hexImage.h>
#include <c-linux/hexFile.h>

#include <c-linux/fd.h>
#include <fcntl.h>
#include <unistd.h>

#include <fifo.h>
#include <fifoParse.h>
#include <fifoPrint.h>
#include <stdio.h>

#if 0
#define DEBUG(x) x
static char errBuffer[1000];
static Fifo _fifoErr = {errBuffer, sizeof errBuffer }, *fifoErr = &_fifoErr;

static void flush() {
	while (fifoCanRead(fifoErr)) fdWrite(1,fifoRead(fifoErr));
}
#else
#define DEBUG(x)
#endif

static bool endsWith(const char *name, const char *extension) {
	const size_t extLength = strlen(extension);
	const char *p = strstr(name, extension);
	return p!=0 && (p[extLength]=='\0');
}

static bool isHexFile(const char *name) {
	return endsWith(name,".hex");
}

bool hexFileLoadBin(int fd, HexImage *hexImage) {
	DEBUG( fprintf(stderr,"Loading .bin file."); )
	/*
	hexImageSetSegmentAddress(hexImage,0);
	for (Uint32 b=0; fdCanRead(fd); b++) {
		const Uint8 byte = (Uint8)fdRead(fd);
		if (hexImageWrite(hexImage,b,&byte,1)) ; // fine
		else return false;
	}
	*/
	Segment *segment = &hexImage->segments[0];
	hexImageSetSegmentAddress(hexImage,0);
	hexImage->entryPoint = 0;
	segment->data = hexImage->ram;
	segment->size = read(fd,segment->data, hexImage->ramSize);
	// hexImage->usedSegment = 0; // useless
	return true;
}

bool hexFileLoadHex(int fd, HexImage *hexImage) {
	DEBUG( fprintf(stderr,"Loading .hex file.\n"); )
	char fifoBuffer[1024];
	Fifo fifo = { fifoBuffer, sizeof fifoBuffer };
	Fifo line;

	while(true) {
		while (fdCanRead(fd) && fifoCanWrite(&fifo)) fifoWrite(&fifo,fdRead(fd));
		if (fifoParseLine(&fifo,&line,EOL_CRLF)) {
			HexRecord record;
			if (fifoParseHexRecord(&line,&record)) {
				DEBUG(	
					fifoPrintString(fifoErr,"Parsed record:");
					fifoPrintHexRecord(fifoErr,&record);
					fifoPrintLn(fifoErr);
					flush();
				)
				switch(record.type) {
					case HEX_EXTENDED_SEGMENT_ADDRESS:
						hexImageSetSegmentAddress(hexImage,hexRecordAddressSegmented(&record));
						break;
					case HEX_EXTENDED_LINEAR_ADDRESS:
						hexImageSetSegmentAddress(hexImage,hexRecordAddressLinear(&record));
						break;
					case HEX_DATA:
						if (hexImageWrite(hexImage,record.offset,record.data, record.length)) ; // fine
						else {
							DEBUG( fprintf(stderr,"could not write data.\n"); )
							return false;
						}
						break;
					case HEX_EOF:
						DEBUG( fprintf(stderr,"EOF hex file.\n"); )
						return true;
					case HEX_START_LINEAR_ADDRESS:
						hexImage->entryPoint = hexRecordStartLinear(&record);
						break;
					case HEX_START_SEGMENT:
						hexImage->entryPoint = hexRecordStartSegmented(&record);
						break;
					default:
						DEBUG(
							fifoPrintString(fifoErr,"Cannot handle:");
							fifoPrintHexRecord(fifoErr,&record);
							fifoPrintLn(fifoErr);
							flush();
						)
						return false;
				}
			}
			else {
				DEBUG(	fifoPrintStringLn(fifoErr,"Not a HEX record."); )
				DEBUG( flush(); )
				return false;
			}
		}
		else break;
		DEBUG( flush(); )
	}
	DEBUG( fprintf(stderr,"premature end-of-file.\n"); )
	return false;	
}

bool hexFileLoad(const char *fnImage, HexImage *hexImage) {
	bool success = false;
	const int fdImage = fnImage!=0 ? open(fnImage,O_RDONLY) : 0;
	if (fdImage<0) return false;

	if (isHexFile(fnImage)) {
		success = hexFileLoadHex(fdImage,hexImage);	
	}
	else {
		success = hexFileLoadBin(fdImage,hexImage);
	}
	if (fdImage>0) close(fdImage);
	return success;
}


