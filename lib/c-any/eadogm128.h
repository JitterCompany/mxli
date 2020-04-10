/*
  eadogm128.h 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef eadogm128_h
#define eadogm128_h

/** @file
 * @brief Generic functions for driving a EA DOGM128 module with SPI.
 *
 */

/** Initializes the ST7565 controller.
 */
int eadogm128Init(void);

/** Blocking function.
 */
void eadogm128Line(int line);

/** Blocking function.
 */
void eadogm128Page(int page);

/** Blocking function.
 */
void eadogm128Column(int column);

enum {
	CLEAR_BLACK	=0,
	CLEAR_CHESS1	=0x55AA55AA,
	CLEAR_CHESS2	=0x3333CCCC,
	CLEAR_WHITE	=0xFFFFFFFF,
};

/** Non-Blocking function.
 */
int eadogm128Clear(int pattern8);

#endif
