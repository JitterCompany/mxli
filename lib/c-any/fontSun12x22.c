/*
  fontSun12x22.c 
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

/* Linux kernel Sun12x22 font */

#include <fonts.h>

#include "fontSun12x22.inc"

static const Gfxmono imageBuffer = {
	12, 256*22,	// nX, nY
	1,16,		// unitX, unitY
	(void*) fontData
};

static CharLocation charLocation(char c) {
	const CharLocation charLocation = { 0, c*22, 12 };

	return charLocation;
}

static CharLocation charLocationV(char c) {
	return fontCharLocationVariableLength(&fontSun12x22,c,6,2);
}

const Font fontSun12x22 = { &imageBuffer, 22, &charLocation };
const Font fontSun12x22V = { &imageBuffer, 22, &charLocationV };

