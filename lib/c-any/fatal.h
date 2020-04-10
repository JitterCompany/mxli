/*
  fatal.h 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef fatal_h
#define fatal_h

/** @file
 * @brief Fatal error indicator interface.
 *
 * This module provides a weak implementation of the fatal function. On a real system, the message should be printed
 * to an output device and the program halted.
 */

void fatal(const char *msg);

#endif
