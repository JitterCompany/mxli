/*
  fb128x64.c
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

//HEADER
#include <fb128x64.h>
#include <dogDriver.h>
#include <st7565.h>
#include <string.h>

enum {
	ONES	=(Uint64)(-1ll),
};

//SLICE
Uint64	fb128x64Buffer[128];
Gfxmono	fb128x64 = { 128,64, 64,1, fb128x64Buffer };

//SLICE
void fb128x64Chess(int w, bool colorBg) {
	Uint64 pattern = 0;
	for (int i=0; i<64; ++i) if (w && (i/w) & 1) pattern |= 1llu<<i;
	if (colorBg) pattern = ~pattern;
	for (int x=0; x<128; ++x) fb128x64Buffer[x] = w && x/w & 1 ? pattern : ~pattern;
}

//SLICE
void fb128x64Clear(bool colorBg) {
	const Uint64 pattern = colorBg ? ONES : 0;
	for (int x=0; x<128; ++x) fb128x64Buffer[x] = pattern;
}

//SLICE
void fb128x64ScrollY(int nY, bool colorBg) {
	const Uint64 pattern = nY >=0 ?
		(colorBg ? ONES >> 64-nY : 0)
		:
		(colorBg ? ONES << 64+nY : 0);

	for (int x=0; x<128; ++x) {
		if (nY>=0) fb128x64Buffer[x] = fb128x64Buffer[x] << nY | pattern;
		else fb128x64Buffer[x] = fb128x64Buffer[x] >> -nY | pattern;
	}
}

//SLICE
void fb128x64ScrollX(int nX, bool colorBg) {
	const Uint64 pattern = colorBg ? ONES : 0;
	if (nX>=0) for (int x=127; x>=0; --x) fb128x64Buffer[x] = x-nX >= 0 ? fb128x64Buffer[x-nX] : pattern;
	else for (int x=0; x<128; ++x) fb128x64Buffer[x] = x-nX < 128 ? fb128x64Buffer[x-nX] : pattern;
}

