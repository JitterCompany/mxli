/*
  int32Scale.h 
  Copyright 2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef int32Scale_h
#define int32Scale_h

#include <integers.h>
#include <int32Math.h>
#include <int64Math.h>

/** @file
 * @brief Toolkit for doing numerical integration in a sampled system with mostly 32bit operations.
 *
 * This module is intended for 32bit CPUs. 64bit divide is used in initialization functions, only.
 *
 * Int32Scale allows precision scaling of values, for example scaling by 1/fsample in the numerical int32Scale.
 * Keep in mind, that the values may well be E10 or E20 fixed point values instead of integers without a change in
 * function calls.
 */

/** Object for scaling int32-values with high precision.
 */
typedef struct {
	Int32	m;		///< full integer range
	Int8	e;		///< range 0..31
} Int32Scale;

/** Fixed point value: 32bits integral part, 32bit fractional part.
 * Atomic access to value OR fraction, but never both.
 */
typedef union {
	struct {
		Uint32	fraction;
		Int32	value;
	};
	Int64		value_e32;
} Q3232;

/** Initializes a scaler with a ratio p/q. This is an approximation. Do not expect scaling to be exact.
 * @param scale the opject to initialize.
 * @param mul the multiplier p
 * @param div the divider q
 */
inline static void int32ScaleInit(Int32Scale *scale, int mul, int div) {
	//const int eF = 31 + uint32Log2Floor(div) - uint32Log2Ceil(mul);	// causes sign errors for mul = div = 1
	const int eF = 30 + uint32Log2Floor(div) - uint32Log2Ceil(mul);
	scale->m = int64Div((Int64)mul<<eF,div);
	scale->e = eF;
}

/** Scales an integral value, resulting in a Q32.32 value.
 * @param scale the scaler object
 * @param value an integral / Exx value withing the Int32 range.
 * @return the scaled value as a pair or integral / Exx part and the fractional part of that value. Normally the
 *   fractional part is discarded, because of its over-preciseness.
 */
inline static Q3232 int32ScaleValue(const Int32Scale *scale, Int32 value) {
	Q3232 p = {
		{
			.value = (Int64)value * scale->m >> scale->e,
			.fraction = int64MulExp2((Int64)value * scale->m,32 - scale->e)
		}
	};
	return p;
}

/* Scales forward the fraction of a value.
inline static Int32 int32ScaleFraction(const Int32Scale *s, Int32 fraction) {
	return int64MulExp2( fraction, 32 - s->e) * s->m;
}
*/

#endif
