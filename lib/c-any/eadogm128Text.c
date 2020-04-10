/*
  eadogm128Text.c 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <stdbool.h>
#include <gfxmono.h>
#include <dogDriver.h>
#include <eadogm128.h>
#include <consoleText.h>
#include <string.h>
#include <fatal.h>

/** Extremely fast text-mode console for 8-pixel fonts.
 */
struct {
	char 		pixelBuffer[128*64/8];
	Gfxmono		imageBuffer;
	int		x;
	int		y;
	int		scrollY;
	int		tab;
	//const Font	*font;
	bool		upsideDown;		// special handling needed in this mode
	bool		autoFlush;		// flush after every char (slower)
	//int		dirtyX0;
	//int		dirtyX1;
} eadogm128Text = {
	.pixelBuffer = { },
	.imageBuffer = { 128, 8, 8,1, eadogm128Text.pixelBuffer },
	//.font = &font8x8V,
	.x = 0, .y = 0, .scrollY = 0,
	.upsideDown = false,
	.autoFlush = false
};

static void eadogm128TextHome(void);
static void eadogm128TextFlush(void);

void eadogm128TextInit(bool autoFlush) {
	const Font* font = consoleGetFont();
	if (font!=0 && font->nY==8) { 
		eadogm128TextHome();
		eadogm128Text.scrollY = 0;
		eadogm128Text.autoFlush = autoFlush;
	}
	else fatal("Console font missing or invalid.");
}

static void eadogm128TextHome(void) {
	eadogm128TextFlush();
	eadogm128Text.x = 0;
	eadogm128Text.y = 0;
	eadogm128Text.tab = 0;
}

static void eadogm128TextClear(void) {
	eadogm128Clear(0);
	memset(eadogm128Text.pixelBuffer, 0, sizeof eadogm128Text.pixelBuffer);
	eadogm128Text.x = 0;
	eadogm128Text.y = 0;
	eadogm128Text.tab = 0;
}

/** Transfers the line buffer contents to the display.
 */
static void eadogm128TextFlush(void) {
	eadogm128Page((eadogm128Text.scrollY+eadogm128Text.y)/8);
	eadogm128Column(eadogm128Text.upsideDown ? 4 : 0);
	int tid = 0;
	do { tid = dogDriverWriteData(eadogm128Text.pixelBuffer,128); } while (!tid);
	dogDriverWaitFor(tid);
}

/** Flushes line buffer and scrolls display up, if neccessary.
 */
static void eadogm128TextLf(void) {
	eadogm128TextFlush();
	eadogm128Text.x = 0;
	eadogm128Text.tab = 0;
	if (eadogm128Text.y+8<64) {	// (over-)writing from to to bottom
		// copy line buffer into LCD RAM
		eadogm128Text.y += 8;
	}
	else {		// scroll up and stay in last line
		eadogm128Text.scrollY = (eadogm128Text.scrollY + 8) & 63;
		eadogm128Line(eadogm128Text.scrollY);
	}
	// clear line buffer
	memset(eadogm128Text.pixelBuffer, 0, sizeof eadogm128Text.pixelBuffer);
	eadogm128TextFlush();
}
static void eadogm128TextCr(void) {
	eadogm128Text.x = 0;
	eadogm128Text.tab = 0;
}

static void eadogm128TextClearEol(void) {
	memset(&eadogm128Text.pixelBuffer[eadogm128Text.x], 0, sizeof eadogm128Text.pixelBuffer -eadogm128Text.x);
	if (eadogm128Text.autoFlush) eadogm128TextFlush();
}

// printable chars only
static void eadogm128TextPrint(char c) {
	const int x = eadogm128Text.x;
	eadogm128Text.x += gfxmonoDrawChar(&eadogm128Text.imageBuffer,x,0,consoleGetFont(),c,true);
	const bool unseenPixels = eadogm128Text.x>128;
	if (eadogm128Text.x>=128) eadogm128TextLf();
	if (unseenPixels) {
		eadogm128Text.x = x - 128;
		eadogm128Text.x = x - 128 + gfxmonoDrawChar(
			&eadogm128Text.imageBuffer,
			x - 128, 0,
			consoleGetFont(),c,true
		);
		if (!eadogm128Text.autoFlush) eadogm128TextFlush();	// show this, if not shown by auto-Flush
	}
	if (eadogm128Text.autoFlush) eadogm128TextFlush();	// show this, if not shown by auto-Flush
}

static void eadogm128TextTab(void) {
	eadogm128Text.tab++;
	const int newX = consoleGetTabStop(eadogm128Text.tab);
	if (newX<128) eadogm128Text.x = newX;
}

// implementation of consoleText interface function
void consolePrintChar(char c) {
	switch(c) {
		case '\001': eadogm128TextFlush(); break;
		case '\n': eadogm128TextLf(); break;
		case '\r': eadogm128TextCr(); break;
		case '\f': eadogm128TextClear(); break;
		case '\b': eadogm128TextClearEol(); break;
		case '\v': eadogm128TextHome(); break;
		case '\t': eadogm128TextTab(); break;
		default  : eadogm128TextPrint(c);
	}
}

