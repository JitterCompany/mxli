/*
  st7565.h 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef st7565_h
#define st7565_h

/** @file
 * @brief Programming of Sitronix 132x64 LCD controller ST7565.
 *
 */

enum St7565Commands {
	DISPLAY_POWER		=0xAE,		///< +0=off, +1=on
	DISPLAY_SCROLL_Y	=0x40,		///< +line=0..63
	DISPLAY_PAGE		=0xB0,		///< +page=0..7
	DISPLAY_COLUMN		=0x10,		///< +4MSB column, extra byte 4LSB column, column=0..131
	DISPLAY_REVERSE_X	=0xA0,		///< +0=normal, +1=reverse
	DISPLAY_INVERT		=0xA6,		///< +0=normal, +1=reverse
	DISPLAY_ALL_ON		=0xA4,		///< +0=normal, +1=all on
	DISPLAY_LCD_BIAS	=0xA2,		///< +0=1/9, +1=1/7
	DISPLAY_STATIC_IND	=0xAC,		///< +0=off, +1=on, second byte= 0/1:flashing mode
	DISPLAY_RESET		=0xE2,		///< internal reset.
	DISPLAY_REVERSE_Y	=0xC0,		///< +0=normal, +8=reverse
	DISPLAY_POWER_MODE	=0x28,		///< +mode=0..7
	DISPLAY_VREG		=0x20,		///< +ratio=0..7
	DISPLAY_VLCD		=0x81,		///< second byte v=0..63
	DISPLAY_BOOSTER		=0xF8,		///< second byte: 0:2x,3x,4x, 1:5x, 3:6x
	DISPLAY_NOP		=0xE3,
};

#endif
