/*
  consoleText.h 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef consoleText__h
#define consoleText__h

/** @file
 * @brief Text mode console abstraction.
 */

#include <fonts.h>

/** Needed to install a weak dummy function consolePrintChar.
 * @param font the Font to use. For some consoles, the font is not selectable and may be 0 in that case.
 * @param tab the tabulator position function. If set to 0, then the default function will be used. On a graphic
 *   console this function has a resultion of one pixel, for text-only consoles, the unit is one character.
 */
void consoleInterfaceInit(const Font *font, int (*tab)(int n));

/** Text output interface.
 */
void consolePrintChar(char c);

/** Retrieves the currently used font.
 * @return the selected font (graphics console) or 0 in case of a text-only output device.
 */
const Font* consoleGetFont(void);

/** Calculates the x-position of a tab-stop.
 * @param n the tab stop position. leftmost is 0.
 * @return in case of a graphics console the pixel position of tab position n. In case of a text-only console
 *   the character position is returned (zero-based).
 */
int consoleGetTabStop(int n);

#endif
