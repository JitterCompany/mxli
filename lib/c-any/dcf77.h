/*
  dcf77.h 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __dcf77_h
#define __dcf77_h

#include <timeAndDate.h>
#include <stddef.h>

/** @file
 * @brief Generic DCF77 types and functionality.
 *
 * DCF77 bit pattern decoding can be separated from any hardware-depended signal aquisition.
 * Communication between hardware and this module is done with a 64-bit integer. Each time hardware decoded a full
 * DCF77-record (one minute of bits) the functions below should be called to validate and extract time information.
 * The bit buffer is filled as follows: second 0 data at bit position 0, second 1 data at bit position 1 and so on.
 * Up to 59 bits can be available (second 0 to second 58). If less than 59 bits are valid, then the lower part of
 * bits is undefined. The higher bits can still be valid and must be provided at correct positions.
 */

/** DCF77 provides 59 bits record each second.
 */
typedef unsigned long long Dcf77Time;

/** Extracts the current time.
 * @param time the destination buffer. Filled even, if parity error.
 * @param dt 59 bit record of DCF77.
 * @param validBits how many bits are assumed to be valid?
 * @return true if successfully parsed and parity OK.
 */
bool dcf77ExtractTime(Time *time, Dcf77Time dt, size_t validBits);

/** Extracts the current date.
 * @param date the destination buffer. Filled even, if parity error.
 * @param dt 59 bit record of DCF77.
 * @param validBits how many bits are assumed to be valid?
 * @return true if successfully parsed and parity OK.
 */
bool dcf77ExtractDate(Date *date, Dcf77Time dt, size_t validBits);

#endif

