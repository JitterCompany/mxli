/*
  crc.h 
  Copyright 2014 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __crc_h
#define __crc_h

#include <integers.h>

/** @file
 * @brief CRC calculation.
 *
 * CRC calculation is started by setting the initial shift register. After that, processing is performed one byte at a
 * time.
 */

enum {
	CRCPOLY_8_ITUT	=1 | 1<<1 | 1<<2 | 1<<8,
	CRCPOLY_8_1WIRE	=1 | 1<<4 | 1<<5 | 1<<8,
};

/** Processes another byte.
 * @param polynomial the CRC polynomial (not neccessarily containing the highest power (of 8).
 * @param shiftRegister the current value of the shift register.
 * @param data the data to add to the stream.
 * @return the current division remainder - the CRC sum, finally.
 */
Uint32 crc8Feed (Uint32 polynomial, Uint32 shiftRegister, Uint8 data);

Uint32 crc8FeedN (Uint32 polynomial, Uint32 shiftRegister, const Uint8 *data, Uint32 n);

#endif

