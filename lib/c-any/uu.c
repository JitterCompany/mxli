/*
  uu.c
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <uu.h>

static inline int min32(int a, int b) {
	return a<b ? a : b;
}

static char uuEscape(char c) {
	return c!=0 ? c+0x20 : 0x60;
}

static char uuUnEscape(char c) {
	return c!=0x60 ? c-0x20 : 0;
}

static int uuReadTriple(Fifo *fifo, Uint32 *checksum) {
	int bytes[3] = { 0,0,0 };
	for (int b=0; b<3 && fifoCanRead(fifo); ++b) {
		bytes[b] = fifoRead(fifo);
		if (checksum) *checksum += bytes[b] & 0xFF;
	}
	return
		( bytes[0]>>2 & 0x3F )<<0
		| ( bytes[0]<<4 & 0x30 | bytes[1]>>4 & 0x0F )<<8
		| ( bytes[1]<<2 & 0x3C | bytes[2]>>6 & 0x03 )<<16
		| ( bytes[2] & 0x3F )<<24
		;
}

/** UUencodes one line of output (45 input bytes max).
 * @return true, if output FIFO was large enough.
 */
bool fifoUuEncodeLine(Fifo *uu, Fifo *data, Uint32 *checksum) {
	bool success = true;
	// maximum of 45 input bytes
	const int nBytes = min32(45,fifoCanRead(data));
	success = success && fifoPrintChar(uu,uuEscape(nBytes));
	for (int triple=0; triple*3 < nBytes; ++triple) {
		const int values = uuReadTriple(data,checksum);
		for (int i=0; i<4; ++i) 
			success = success && fifoPrintChar(uu,uuEscape(values>>i*8 & 0xFF));
	}
	return success && fifoPrintString(uu,"\r\n");
}

bool fifoUuDecodeLine(Fifo *data, Fifo *uu, Uint32 *checksum) {
	if (!fifoCanRead(uu)) return false;
	bool success = true;
	int nBytes = uuUnEscape(fifoRead(uu));
	for (int b=0; b<nBytes; ) {
		char chars[4] = { 0,0,0,0 };
		for (int c=0; c<4 && fifoCanRead(uu); ++c) chars[c] = uuUnEscape(fifoRead(uu));
		const char bytes[3] = {
			chars[0]<<2 | chars[1]>>4 & 0x03,
			chars[1]<<4 | chars[2]>>2 & 0x0F,
			chars[2]<<6 | chars[3]
		};
		for (int b3=0; b3<3 && b<nBytes; ++b3, ++b) {
			success = success && fifoPrintChar(data,bytes[b3]);
			if (checksum) *checksum += bytes[b3] & 0xFF;
		}
	}
	return success;
}

