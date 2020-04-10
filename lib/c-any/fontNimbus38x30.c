/*
  fontNimbus38x30.c 
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
#include <fonts.h>

#include "fontNimbus38x30.inc"

static const Gfxmono imageBuffer = {
	38*256, 30,
	32,1,
	(void*) fontData
};

static CharLocation charLocation(char c) {
	const CharLocation charLocation = { c*38, 0, 38 };

	return charLocation;
}

static CharLocation charLocationV(char c) {
	return fontCharLocationVariableLength(&fontNimbus38x30,c,20,2);
}

const Font fontNimbus38x30 = { &imageBuffer, 30, &charLocation };
const Font fontNimbus38x30V = { &imageBuffer, 30, &charLocationV };

