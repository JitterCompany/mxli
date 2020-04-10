/*
  fonts.h 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef fonts_H
#define fonts_H

/** @file
 * @brief Pixel fonts for graphic displays.
 */

#include <gfxmono.h>

/** System 7x8 (char 0 .. #32 to #127). 7 columns of 8 bits per char. */
extern const Font font7x8;
extern const Font font7x8;

extern const Font font8x8;
extern const Font font8x8V;

extern const Font fontVga6x11;
extern const Font fontVga6x11V;

extern const Font fontVga8x8;
extern const Font fontVga8x8V;

extern const Font fontSun8x16;
extern const Font fontSun8x16V;

extern const Font fontSun12x22;
extern const Font fontSun12x22V;

extern const Font fontNimbus38x30;
extern const Font fontNimbus38x30V;

#endif
