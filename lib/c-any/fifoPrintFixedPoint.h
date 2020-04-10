/*
  fifoPrintFixedPoint.h 
  Copyright 2014-2015 Marc Prager
 
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
 * This module provides fixed point decimal output functions of 16bit and 32bit values.
 * Functions printing 16bit values are most likely faster.
 * It is assumed that for most programs only one type of fixed point values is used. Therefore the functions differing
 * in decimal point position do not share a common generic function.
 * Where possible, function take at most 4 parameters to keep parameter passing simple for the user and the compiler.
 * Only the rarely directly used function printXxxxPrefix may violate this restriction.
 */

#ifndef __fifoPrintFixedPoint_h
#define __fifoPrintFixedPoint_h

#include <integers.h>
#include <fixedPoint.h>
#include <fifoPrint.h>

bool fifoPrintUint16 (Fifo *fifo, Uint32 value, int minWidth);
bool fifoPrintUint16Pad (Fifo *fifo, Uint32 value, int minWidth, char padChar);

bool fifoPrintUint32 (Fifo *fifo, Uint32 value, int minWidth);
bool fifoPrintUint32Pad (Fifo *fifo, Uint32 value, int minWidth, char padChar);

/** Prints an unsigned decimal value with an optional prefix char, in case of a non-zero value.
 * @param fifo the output Fifo object.
 * @param value an unsigned integer within 0 .. 2^16-1
 * @param prefix a prefix char in front of the digits, if this char is not 0.
 * @param minWidth the minimum number of digits + prefix char (if any). A minWidth of 0 results in an empty string if
 *   the value converted is 0.
 */
bool fifoPrintUint16Prefix (Fifo *fifo, Uint32 value, char prefix, int minWidth);

/** Prints an unsigned decimal value.
 * @param fifo the output Fifo object.
 * @param value an unsigned integer within 0 .. 2^32-1
 * @param prefix a prefix char in front of the digits, if this char is not 0.
 * @param minWidth the minimum number of digits + prefix char (if any). A minWidth of 0 results in an empty string if
 *   the value converted is 0.
 */
bool fifoPrintUint32Prefix (Fifo *fifo, Uint32 value, char prefix, int minWidth);

/** Prints an unsigned decimal value.
 * @param fifo the output Fifo object.
 * @param value a signed integer within -2^15 .. +2^15-1
 * @param minWidth the minimum number of digits + prefix char (if any). A minWidth of 0 results in an empty string if
 *   the value converted is 0. Negative minWidth -n always forces a sign in front of the number, even for non-negative
 *   values and the number plus the sign is at least n digits. A value of -1 results in an empty string for value 0.
 */
bool fifoPrintInt16 (Fifo *fifo, Int32 value, int minWidth);

/** Prints a signed decimal value.
 * @param fifo the output Fifo object.
 * @param value a signed integer within -2^31 .. +2^31-1
 * @param minWidth the minimum number of digits + prefix char (if any). A minWidth of 0 results in an empty string if
 *   the value converted is 0. Negative minWidth -n always forces a sign in front of the number, even for non-negative
 *   values and the number plus the sign is at least n digits. A value of -1 results in an empty string for value 0.
 */
bool fifoPrintInt32 (Fifo *fifo, Int32 value, int minWidth);

// E10 fixed point...
//
/** Prints a fixed point, non-negative decimal.
 * @param fifo the output Fifo object.
 * @param value the unsigned value with implicit decimal point between bit 9 and bit 10.
 * @param minWidth the minimum width for the integral part. If this is 0 then values < 1 are
 *   fifoPrinted without a leading zero. 
 * @param precision the exact number of digits and decimal point.
 */
bool fifoPrintUint32_e10(Fifo *fifo, Uint32 value, int minWidth, int precision);

/** Prints a fixed point, non-negative decimal, with an optional prefix char.
 * @param fifo the output Fifo object.
 * @param value the unsigned value with implicit decimal point between bit 9 and bit 10.
 * @param prefix one character placed in front of the number part, if != 0
 * @param minWidth the minimum width for the integral part, including the sign. If this is 0 then values < 1 are
 *   printed without a leading zero. 
 * @param precision the exact number of digits and decimal point.
 */
bool fifoPrintUint32Prefix_e10(Fifo *fifo, Uint32 value, char prefix, int minWidth, int precision);

/** Print a signed fixed point decimal. Decimal point between bit 9 and bit 10.
 * @param fifo the output Fifo object.
 * @param value a fixed point integer, value -2^21 .. +2^21, resolution 1/1024.
 * @param minWidth minimum number of digits (and sign) of the integral part. Negative values prepend a sign even for
 *   positive values. A value of 0 gives the shortest number, without leading 0.
 * @param precision exact number of decimal point and digits after the point. Result is truncated, not rounded.
 */
bool fifoPrintInt32_e10(Fifo *fifo, Int32 value, int minWidth, int precision);

// E16 fixed point...
//
/** Prints a fixed point, non-negative decimal.
 * @param fifo the output Fifo object.
 * @param value the unsigned value with implicit decimal point between bit 9 and bit 10.
 * @param minWidth the minimum width for the integral part. If this is 0 then values < 1 are
 *   printed without a leading zero. 
 * @param precision the exact number of digits and decimal point.
 */
bool fifoPrintUint32_e16(Fifo *fifo, Uint32 value, int minWidth, int precision);

/** Prints a fixed point, non-negative decimal, with an optional prefix char.
 * @param fifo the output Fifo object.
 * @param value the unsigned value with implicit decimal point between bit 9 and bit 10.
 * @param prefix one character placed in front of the number part, if != 0
 * @param minWidth the minimum width for the integral part, including the sign. If this is 0 then values < 1 are
 *   printed without a leading zero. 
 * @param precision the exact number of digits and decimal point.
 */
bool fifoPrintUint32Prefix_e16(Fifo *fifo, Uint32 value, char prefix, int minWidth, int precision);

/** Print a signed fixed point decimal. Decimal point between bit 9 and bit 10.
 * @param fifo the output Fifo object.
 * @param value a fixed point integer, value -2^21 .. +2^21, resolution 1/1024.
 * @param minWidth minimum number of digits (and sign) of the integral part. Negative values prepend a sign even for
 *   positive values. A value of 0 gives the shortest number, without leading 0.
 * @param precision exact number of decimal point and digits after the point. Result is truncated, not rounded.
 */
bool fifoPrintInt32_e16(Fifo *fifo, Int32 value, int minWidth, int precision);

// E20 fixed point...
//
/** Prints a fixed point, non-negative decimal.
 * @param fifo the output Fifo object.
 * @param value the unsigned value with implicit decimal point between bit 9 and bit 10.
 * @param minWidth the minimum width for the integral part. If this is 0 then values < 1 are
 *   printed without a leading zero. 
 * @param precision the exact number of digits and decimal point.
 */
bool fifoPrintUint32_e20(Fifo *fifo, Uint32 value, int minWidth, int precision);

/** Prints a fixed point, non-negative decimal, with an optional prefix char.
 * @param fifo the output Fifo object.
 * @param value the unsigned value with implicit decimal point between bit 9 and bit 10.
 * @param prefix one character placed in front of the number part, if != 0
 * @param minWidth the minimum width for the integral part, including the sign. If this is 0 then values < 1 are
 *   printed without a leading zero. 
 * @param precision the exact number of digits and decimal point.
 */
bool fifoPrintUint32Prefix_e20(Fifo *fifo, Uint32 value, char prefix, int minWidth, int precision);

/** Print a signed fixed point decimal. Decimal point between bit 9 and bit 10.
 * @param fifo the output Fifo object.
 * @param value a fixed point integer, value -2^21 .. +2^21, resolution 1/1024.
 * @param minWidth minimum number of digits (and sign) of the integral part. Negative values prepend a sign even for
 *   positive values. A value of 0 gives the shortest number, without leading 0.
 * @param precision exact number of decimal point and digits after the point. Result is truncated, not rounded.
 */
bool fifoPrintInt32_e20(Fifo *fifo, Int32 value, int minWidth, int precision);

/** Most common output format: minimum number of integral digits, but a single leading zero, if neccessary.
 */
bool fifoPrintInt32Easy_e20(Fifo *fifo, Int32 value, int precision);

////////////////////////////////////////////////////////////////////////////////////////////////////
// any other positions...

/** Print a signed fixed point decimal. 
 * @param fifo the output Fifo object.
 * @param e the position of the binary point.
 * @param value a fixed point integer
 * @param prefix a char to print in front of the numer (a sign, for example ;-) or nothing if ==0.
 * @param minWidth the minimum number of integral and sign chars.
 * @param precision exact number of decimal point and digits after the point. Result is truncated, not rounded.
 */
bool fifoPrintUint32Prefix_en (Fifo *fifo, int e, Uint32 value, char prefix, int minWidth, int precision);

/** Print a signed fixed point decimal. 
 * @param fifo the output Fifo object.
 * @param e the position of the binary point.
 * @param value a fixed point integer
 * @param minWidth the minimum number of integral and sign chars.
 * @param precision exact number of decimal point and digits after the point. Result is truncated, not rounded.
 */
inline static bool fifoPrintUint32_en (Fifo *fifo, int e, Uint32 value, int minWidth, int precision) {
	return fifoPrintUint32Prefix_en (fifo,e,value,0,minWidth,precision);
}

/** Print a signed fixed point decimal. 
 * @param fifo the output Fifo object.
 * @param e the position of the binary point.
 * @param value a fixed point integer
 * @param minWidth the minimum number of integral and sign chars.
 * @param precision exact number of decimal point and digits after the point. Result is truncated, not rounded.
 */
bool fifoPrintInt32_en (Fifo *fifo, int e, Int32 value, int minWidth, int precision);

#endif
