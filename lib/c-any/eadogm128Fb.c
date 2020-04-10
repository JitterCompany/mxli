/*
  eadogm128Fb.c
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <eadogm128Fb.h>
#include <dogDriver.h>
#include <st7565.h>
#include <string.h>

Uint64	graphicsBuffer[128];
Gfxmono	frameBuffer = { 128,64, 64,1, graphicsBuffer };

/* Copies the graphicsBuffer into a ST7565 RAM. Scanning direction is page by page of ST7565: 8 pages a 128 bytes
 * each. graphicsBuffer is in column order.
 *
 */
DogByte callbackFrameBuffer(const char *data, int index, int n, int subIndex) {
	const int absoluteIndex = (subIndex>>16) + index;
	const int byteX = absoluteIndex & 127;
	const int byteY = absoluteIndex >> 7;
	const char dataByte = data[byteY+byteX*8];
	if (absoluteIndex & 127 && index!=0) return DOG_DATA | dataByte;
	else {	// page needs to be selected first, then column
		// 3 control bytes
		switch(subIndex & 3) {
			case 0:	return DOG_PREFIX | DOG_CMD | DISPLAY_PAGE | (absoluteIndex/128) & 7;	// page select
			case 1:	return DOG_PREFIX | DOG_CMD | DISPLAY_COLUMN | 0;	// column MSB4 of 0
			case 2: return DOG_PREFIX | DOG_CMD | 0;			// column LSB4 of 0
			default: return DOG_DATA | dataByte;		// finally the data byte itself
		}
	}
}

int eadogm128FbUpdate(void) {
	return dogDriverStartTransfer(graphicsBuffer,sizeof graphicsBuffer, &callbackFrameBuffer, 0);
}

void eadogm128FbChess(int w) {
	Uint64 pattern = 0;
	for (int i=0; i<64; ++i) if (w && (i/w) & 1) pattern |= 1llu<<i;
	for (int x=0; x<128; ++x) graphicsBuffer[x] = w && x/w & 1 ? pattern : ~pattern;
}

void eadogm128FbClear(bool color) {
	memset(graphicsBuffer, color?0xFF:0x00, sizeof graphicsBuffer);
}

void eadogm128FbScrollY(int nY) {
	for (int x=0; x<128; ++x) {
		if (nY>=0) graphicsBuffer[x] = graphicsBuffer[x] << nY;
		else graphicsBuffer[x] = graphicsBuffer[x] >> -nY;
	}
}

void eadogm128FbScrollX(int nX) {
	if (nX>=0) for (int x=127; x>=0; --x) graphicsBuffer[x] = x-nX >= 0 ? graphicsBuffer[x-nX] : 0;
	else for (int x=0; x<128; ++x) graphicsBuffer[x] = x-nX < 128 ? graphicsBuffer[x-nX] : 0;
}

