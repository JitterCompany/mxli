/*
  fbConsole.h 
  Copyright 2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef fbConsole_h
#define fbConsole_h

/** @file
 * @brief A text console on top of fb128x64
 *
 * This module provides text output and positioning on a frame buffer device. For now a 8x8 pixel font is used and
 * fb128x64 is assumed as drawing device.
 */

#include <integers.h>
#include <fb128x64.h>
#include <fifo.h>

void fbConsoleInit(bool colorFg);

void fbConsoleClear(void);

/** Maps character position to pixel position.
 * @param x the character x position. 0 is the leftmost position, 1 is one character right, ...
 * @return the pixel position in the frame buffer.
 */
static inline int fbConsoleX(int x) { return x*8; }

/** Maps character position to pixel position.
 * @param y the character y position. 0 is the topmost position, 1 is one character down, ...
 * @return the pixel position in the frame buffer.
 */
static inline int fbConsoleY(int y) { return y*8; }

/** Writes a character at character position x,y.
 * @param x the 0-based character column
 * @param y the 0-based character line number
 * @param c a character
 * @return the character column after writing the char.
 */
int fbConsoleXyChar(int x, int y, char c);

/** Writes a string at the given column,row position.
 * @param x the 0-based character column
 * @param y the 0-based character line number
 * @param text a 0-terminated character string to write.
 * @return the character column after writing the string.
 */
int fbConsoleXyString(int x, int y, const char* text);

/** Writes a Fifo's contents at the given column,row position.
 * @param x the 0-based character column
 * @param y the 0-based character line number
 * @param text a Fifo containing the text to write.
 * @return the character column after writing the string.
 */
int fbConsoleXyFifo(int x, int y, Fifo *text);

/** Clears a limited area within a line.
 * @param x the character to start clearing from
 * @param y the line number
 * @param xTo the end of the clearing region - not including this char.
 */
void fbConsoleXClear(int x, int y, int xTo);

/** Clears until end-of-line.
 * @param x the character to start clearing from
 * @param y the line number
 */
void fbConsoleEolClear(int x, int y);

/** Writes a string at a given x/y-position and clears until the end of line after it.
 * @param x the character to start printing from
 * @param y the line number
 * @param text the output text
 */
static inline void fbConsoleXyStringEolClear(int x, int y, const char* text) {
	fbConsoleEolClear(
		fbConsoleXyString(x,y,text),
		y
	);
}

/** Writes a Fifo's contents at a given x/y-position and clears until the end of line after it.
 * @param x the character to start printing from
 * @param y the line number
 * @param text the output text
 */
static inline void fbConsoleXyFifoEolClear(int x, int y, Fifo* text) {
	fbConsoleEolClear(
		fbConsoleXyFifo(x,y,text),
		y
	);
}
/** Scrolls down the contents efficiently.
 * @param nY the number of characters to move the contents down. Negative for upward movement.
 */
void fbConsoleScrollY(int nY);

/** Print characters and do interpret control sequences like carriage return, line feed and form feed.
 * In addition to the three codes above, fbConsole supports direct x (code 0x80+x) or y (0x90+y) positioning.
 * Clear to end-of-line is 0x03 (ETX).
 * @param c the ASCII or control character.
 */
void fbConsolePrintChar(char c);

void fbConsolePrintString(const char *text);
void fbConsolePrintFifo(Fifo *fifo);

#endif

