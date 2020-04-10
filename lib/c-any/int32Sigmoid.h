/*
  int32Sigmoid.h 
  Copyright 2016 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef int32Sigmoid_h
#define int32Sigmoid_h

#include <integers.h>
#include <int32Math.h>
#include <int64Math.h>

/** @file
 * @brief Toolkit for doing numerical smoothing using (fixed point) sigmoid functions.
 *
 */

/** Calculates a smooth transition from 0 to 1 (E10) in the range 0..1 (E10).
 * Values beyond this range are clipped. Formula f(x) = x^2*3 - x^3*2 .
 */
inline static Uint32 uint32Sigmoid_e10 (Uint32 x_e10) {
	if (x_e10>=E10) return E10;
	else {
		return	(x_e10*x_e10*3 - (x_e10*x_e10*x_e10 >> 9)) >> 10;
	}
}

/** Calculates a smooth transition from 0 to 1 (E20) in the range 0..1 (E20).
 * Values beyond this range are clipped. Formula f(x) = x^2*3 - x^3*2 .
 */
inline static Uint32 uint32Sigmoid_e20 (Uint32 x_e20) {
	if (x_e20>=E20) return E20;
	else {
		return	((Uint64)x_e20*x_e20*3 - ((Uint64)x_e20*x_e20*x_e20 >> 19)) >> 20;
	}
}

#endif
