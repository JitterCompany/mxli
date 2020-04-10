/*
  printFixedPoint.h 
  Copyright 2014 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

/** @file
 * @brief Fixed point decimal output functions.
 *
 */

#ifndef __printFixedPoint_h
#define __printFixedPoint_h

#include <integers.h>
#include <fixedPoint.h>
#include <print.h>
#include <uint16Div.h>
#include <uint32Div.h>

/** Prints an unsigned decimal value. This function can be expected to be faster than printUint32() with the same
 * value due to a faster division function.
 * @param value an unsigned integer within 0 .. 2^16-1
 * @param minWidth the minimum number of digits + prefix char (if any). A minWidth of 0 results in an empty string if
 *   the value converted is 0.
 */
void printUint16 (Uint32 value, int minWidth);

/** Prints an unsigned decimal value.
 * @param value an unsigned integer within 0 .. 2^32-1
 * @param minWidth the minimum number of digits + prefix char (if any). A minWidth of 0 results in an empty string if
 *   the value converted is 0.
 */
void printUint32 (Uint32 value, int minWidth);

/** Prints an unsigned decimal value.
 * @param value an unsigned integer within 0 .. 2^16-1
 * @param prefix a prefix char in front of the digits, if this char is not 0.
 * @param minWidth the minimum number of digits + prefix char (if any). A minWidth of 0 results in an empty string if
 *   the value converted is 0.
 */
void printUint16Prefix (Uint32 value, char prefix, int minWidth);

/** Prints an unsigned decimal value.
 * @param value an unsigned integer within 0 .. 2^32-1
 * @param prefix a prefix char in front of the digits, if this char is not 0.
 * @param minWidth the minimum number of digits + prefix char (if any). A minWidth of 0 results in an empty string if
 *   the value converted is 0.
 */
void printUint32Prefix (Uint32 value, char prefix, int minWidth);

/** Prints an unsigned decimal value.
 * @param value a signed integer within -2^15 .. +2^15-1
 * @param minWidth the minimum number of digits + prefix char (if any). A minWidth of 0 results in an empty string if
 *   the value converted is 0. Negative minWidth -n always forces a sign in front of the number, even for non-negative
 *   values and the number plus the sign is at least n digits. A value of -1 results in an empty string for value 0.
 */
void printInt16 (Int32 value, int minWidth);

/** Prints an unsigned decimal value.
 * @param value a signed integer within -2^31 .. +2^31-1
 * @param minWidth the minimum number of digits + prefix char (if any). A minWidth of 0 results in an empty string if
 *   the value converted is 0. Negative minWidth -n always forces a sign in front of the number, even for non-negative
 *   values and the number plus the sign is at least n digits. A value of -1 results in an empty string for value 0.
 */
void printInt32 (Int32 value, int minWidth);

// E10 fixed point...
//
/** Prints a fixed point, non-negative decimal.
 * @param value the unsigned value with implicit decimal point between bit 9 and bit 10.
 * @param minWidth the minimum width for the integral part. If this is 0 then values < 1 are
 *   printed without a leading zero. 
 * @param precision the exact number of digits and decimal point.
 */
void printUint32_e10(Uint32 value, int minWidth, int precision);

/** Prints a fixed point, non-negative decimal, with an optional prefix char.
 * @param value the unsigned value with implicit decimal point between bit 9 and bit 10.
 * @param prefix one character placed in front of the number part, if != 0
 * @param minWidth the minimum width for the integral part, including the sign. If this is 0 then values < 1 are
 * printed without a leading zero. 
 * @param precision the exact number of digits and decimal point.
 */
void printUint32Prefix_e10(Uint32 value, char prefix, int minWidth, int precision);

/** Print a signed fixed point decimal. Decimal point between bit 9 and bit 10.
 * @param value a fixed point integer, value -2^21 .. +2^21, resolution 1/1024.
 * @param minWidth minimum number of digits (and sign) of the integral part. Negative values prepend a sign even for
 *   positive values. A value of 0 gives the shortest number, without leading 0.
 * @param precision exact number of decimal point and digits after the point. Result is truncated, not rounded.
 */
void printInt32_e10(Int32 value, int minWidth, int precision);

// E16 fixed point...
//
/** Prints a fixed point, non-negative decimal.
 * @param value the unsigned value with implicit decimal point between bit 9 and bit 10.
 * @param minWidth the minimum width for the integral part. If this is 0 then values < 1 are
 *   printed without a leading zero. 
 * @param precision the exact number of digits and decimal point.
 */
void printUint32_e16(Uint32 value, int minWidth, int precision);

/** Prints a fixed point, non-negative decimal, with an optional prefix char.
 * @param value the unsigned value with implicit decimal point between bit 9 and bit 10.
 * @param prefix one character placed in front of the number part, if != 0
 * @param minWidth the minimum width for the integral part, including the sign. If this is 0 then values < 1 are
 * printed without a leading zero. 
 * @param precision the exact number of digits and decimal point.
 */
void printUint32Prefix_e16(Uint32 value, char prefix, int minWidth, int precision);

/** Print a signed fixed point decimal. Decimal point between bit 9 and bit 10.
 * @param value a fixed point integer, value -2^21 .. +2^21, resolution 1/1024.
 * @param minWidth minimum number of digits (and sign) of the integral part. Negative values prepend a sign even for
 *   positive values. A value of 0 gives the shortest number, without leading 0.
 * @param precision exact number of decimal point and digits after the point. Result is truncated, not rounded.
 */
void printInt32_e16(Int32 value, int minWidth, int precision);

// E20 fixed point...
//
/** Prints a fixed point, non-negative decimal.
 * @param value the unsigned value with implicit decimal point between bit 9 and bit 10.
 * @param minWidth the minimum width for the integral part. If this is 0 then values < 1 are
 *   printed without a leading zero. 
 * @param precision the exact number of digits and decimal point.
 */
void printUint32_e20(Uint32 value, int minWidth, int precision);

/** Prints a fixed point, non-negative decimal, with an optional prefix char.
 * @param value the unsigned value with implicit decimal point between bit 9 and bit 10.
 * @param prefix one character placed in front of the number part, if != 0
 * @param minWidth the minimum width for the integral part, including the sign. If this is 0 then values < 1 are
 * printed without a leading zero. 
 * @param precision the exact number of digits and decimal point.
 */
void printUint32Prefix_e20(Uint32 value, char prefix, int minWidth, int precision);

/** Print a signed fixed point decimal. Decimal point between bit 9 and bit 10.
 * @param value a fixed point integer, value -2^21 .. +2^21, resolution 1/1024.
 * @param minWidth minimum number of digits (and sign) of the integral part. Negative values prepend a sign even for
 *   positive values. A value of 0 gives the shortest number, without leading 0.
 * @param precision exact number of decimal point and digits after the point. Result is truncated, not rounded.
 */
void printInt32_e20(Int32 value, int minWidth, int precision);

#endif
