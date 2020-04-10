/*
  sinq.h - Fixed point sine tables.
  Copyright 2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef sinq_h
#define sinq_h
/** @file
 * @brief Space efficient (compared to full tables) sine tables of fixed point values.
 */

#include <integers.h>

/** Sine function.
 * @param phi 8bit angle, period 256 positive and negative.
 * @return amplitude in the range -128..+128 (symmetric).
 */
Int32 sinq8To7(Int32 phi);

/** Sine function.
 * @param phi 10bit angle, period 1024 positive and negative.
 * @return amplitude in the range -32768..+32768 (symmetric).
 */
Int32 sinq10To15(Int32 phi);

/** Sine function.
 * @param phi 12bit angle, period 4096 positive and negative.
 * @return amplitude in the range -32768..+32768 (symmetric).
 */
Int32 sinq12To15(Int32 phi);

/** Signed 'shift right' with rounding always towards 0. -1>>1 is -1, while sinqInt32ShiftRight(-1,1)==0
 * @param value the number to scale down by a power of 2.
 * @param shift the number of bits to shift right. Must not be negative.
 * @return value/2^shift rounded towards 0.
 */
inline static Int32 sinqInt32ShiftRight(Int32 value, int shift) {
	return value>=0 ? value>>shift : - (-value>>shift);
}

/** Signed 'shift right' with rounding always towards 0. -1>>1 is -1, while sinqInt64ShiftRight(-1,1)==0
 * @param value the number to scale down by a power of 2.
 * @param shift the number of bits to shift right. Must not be negative.
 * @return value/2^shift rounded towards 0.
 */
inline static Int64 sinqInt64ShiftRight(Int64 value, int shift) {
	return value>=0 ? value>>shift : - (-value>>shift);
}

#endif

