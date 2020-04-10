/*
  fb128x64.h 
  Copyright 2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef fb128x64_h
#define fb128x64_h

/** @file
 * @brief A monochrome graphics framebuffer, typically used for the EA-DOGM128 L/M modules.
 *
 * This frame buffer uses a memory layout the suits the EA-DOG driver very well. Some basic graphics functions are
 * provided for convenience.
 */

#include <integers.h>
#include <gfxmono.h>

extern Uint64	fb128x64Buffer[128];	///< data type not volatile - take care of that in ISR's!
extern Gfxmono	fb128x64;

/** Clear the entire area to background color (efficiently).
 * @param colorBg the background color.
 */
void fb128x64Clear(bool colorBg);

/** Checker board pattern.
 * @param w the size of each square.
 * @param colorBg the color of the square at location (0,0).
 */
void fb128x64Chess(int w, bool colorBg);

/** Scrolls down the contents efficiently.
 * @param nY the number of pixels to move the contents down. Negative for upward movement.
 * @param colorBg the color to be used for empty space.
 */
void fb128x64ScrollY(int nY, bool colorBg);

/** Scrolls down the contents efficiently.
 * @param nX the number of pixels to move the contents right. Negative for movement left.
 * @param colorBg the color to be used for empty space.
 */
void fb128x64ScrollX(int nX, bool colorBg);

#endif

