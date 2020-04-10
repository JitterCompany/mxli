/*
  eadogm128Fb.h 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef eadogm128Fb_h
#define eadogm128Fb_h

#include <integers.h>
#include <gfxmono.h>

extern Uint64	graphicsBuffer[128];
extern Gfxmono	frameBuffer;

int eadogm128FbUpdate(void);

void eadogm128FbClear(bool color);

void eadogm128FbChess(int w);

void eadogm128FbScrollY(int nY);

void eadogm128FbScrollX(int nX);

#endif

