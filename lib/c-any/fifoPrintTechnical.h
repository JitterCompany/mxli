/*
  fifoPrintTechnical.h 
  Copyright 2016 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

/** @file
 * @brief Decimal output functions using technical prefixes (n=nano, u=micro, ...).
 *
 * This module provides fixed point decimal output functions of 16bit and 32bit values.
 * Functions printing 16bit values are most likely faster.
 * It is assumed that for most programs only one type of fixed point values is used. Therefore the functions differing
 * in decimal point position do not share a common generic function.
 * Where possible, function take at most 4 parameters to keep parameter passing simple for the user and the compiler.
 * Only the rarely directly used function printXxxxPrefix may violate this restriction.
 */

#ifndef __fifoPrintTechnical_h
#define __fifoPrintTechnical_h

#include <integers.h>
#include <fixedPoint.h>
#include <fifoPrint.h>

/** Prints a value including unit and SI prefixes.
 * @param fifo the output
 * @param value the number in units of 10^exponent. E.g. -3 means milli
 * @param exponent decimal point shift.
 * @param unit the physical unit, like "s" for second.
 */
bool fifoPrintTechnicalUint32 (Fifo *fifo, Uint32 value, int exponent, const char *unit);

#endif
