/*
  fontVga6x11.c 
  Copyright 2011 Marc Prager.
  Font data derived from Linux kernel console font files (copyright notice missing there!).
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */
/* Linux kernel VGA6X11 font */

#include <fonts.h>

#include "fontVga6x11.inc"

static const Gfxmono imageBuffer = {
	6, 256*11, 1,8,  (void*) fontData
};

static CharLocation charLocation(char c) {
	const CharLocation charLocation = { 0, c*11, 6 };

	return charLocation;
}

static CharLocation charLocationV(char c) {
	return fontCharLocationVariableLength(&fontVga6x11,c,5,1);
}

const Font fontVga6x11 = { &imageBuffer, 11, &charLocation };
const Font fontVga6x11V = { &imageBuffer, 11, &charLocationV };

