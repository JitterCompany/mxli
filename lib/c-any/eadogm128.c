//HEADER
/*
  eadogm128-sliced.c 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <eadogm128.h>
#include <dogDriver.h>
#include <st7565.h>
#include <integers.h>

int eadogm128Init(void) {
	static const char initSeq[] = {
		DISPLAY_SCROLL_Y | 0,
		DISPLAY_REVERSE_X | 1, DISPLAY_REVERSE_Y | 0,	// EA DOGM128 is made that way
		DISPLAY_INVERT | 0,
		DISPLAY_LCD_BIAS | 0,
		DISPLAY_POWER_MODE | 7,
		DISPLAY_BOOSTER, 0,
		DISPLAY_VREG | 7,
		DISPLAY_VLCD, 22,
		DISPLAY_STATIC_IND | 0, 0,
		DISPLAY_POWER | 1,
	};
	return dogDriverWriteCommand(initSeq,sizeof initSeq);
}

void eadogm128Line(int line) {
	char cmd[] = { DISPLAY_SCROLL_Y | line & 63 };
	int tid = 0;
	do { tid = dogDriverWriteCommand(cmd, sizeof cmd); } while (!tid);
	dogDriverWaitFor(tid);
}

void eadogm128Page(int page) {
	char cmd[] = { DISPLAY_PAGE | page & 7 };
	int tid = 0;
	do { tid = dogDriverWriteCommand(cmd, sizeof cmd); } while (!tid);
	dogDriverWaitFor(tid);
}

void eadogm128Column(int column) {
	char cmd[] = { DISPLAY_COLUMN | 0xF & column>>4, 0xF & column };
	int tid = 0;
	do { tid = dogDriverWriteCommand(cmd, sizeof cmd); } while (!tid);
	dogDriverWaitFor(tid);
}

//SLICE
/**
 * @param subIndex the higher 16-bits are the offset of data from the beginning of the frame buffer origin.
 */
static DogByte callbackWriteFbClear(const char *data, int index, int n, int subIndex) {
	const int absoluteIndex = (subIndex>>16) + index;
	const int pattern = ((int)(Int)data)>>8*(absoluteIndex&3) & 0xFF;
	if (absoluteIndex & 127 && (index!=0)) return DOG_DATA | pattern;
	else {	// page needs to be selected first, then column
		// 3 control bytes
		switch(subIndex & 3) {
			case 0:	return DOG_PREFIX | DOG_CMD | DISPLAY_PAGE | (absoluteIndex/128) & 7;	// page select
			case 1:	return DOG_PREFIX | DOG_CMD | DISPLAY_COLUMN | 0;	// column MSB4 of 0
			case 2: return DOG_PREFIX | DOG_CMD | 0;			// column LSB4 of 0
			default: return DOG_DATA | pattern;		// finally the data byte itself
		}
	}
}

int eadogm128Clear(int pattern8) {
	return dogDriverStartTransfer((const char*)(Int)pattern8,128*64/8,&callbackWriteFbClear,0);
}

