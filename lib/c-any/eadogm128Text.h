/*
  eadogm128Text.h 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef eadogm128Text_h
#define eadogm128Text_h

#include <consoleText.h>

/** Initializes EA-DOGM128 8-pixel text-only console.
 * Module consoleText must be initialized before with a suitable font of fixed or variable length.
 * This function calls fatal() if no font is present.
 * The desired side-effect of this function is the installation of a consolePrintChar() function.
 * @param autoFlush if true, update display after every new char.
 */
void eadogm128TextInit(bool autoFlush);

#endif
