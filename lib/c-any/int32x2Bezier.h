/*
  int32x2Bezier.h 
  Copyright 2016 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */
#ifndef __int32x2Bezier_h
#define __int32x2Bezier_h

/** @file
 * @brief 2-dimensional Bezier curves of order 2 with fixed point calculation only (64bit, 32bit results).
 */

#include <stdbool.h>
#include <integers.h>

/** specialization: Bezier1 (straight line).
 * Fast specialization of Bezier2 direct calculation.
 * 6 multiplications 32x32 -> 64 + additions + shifts.
 * UNTESTED.
 * @param e vector components fixed point position
 * @param et time fixed point position (1..32)
 * @param x0_en Bezier control point
 * @param x1_en Bezier control point
 * @param t_et evaluation time, fixed point. This value may be outside of [0,1] in this function, if et<32.
 */
inline static Int32x2 int32x2Bezier1ValueAt_een (int e, int et, const Int32x2 x0_en, const Int32x2 x1_en, const Int32 t_et) {
	const Int32 s_et = (Int64)((1<<e)-t_et);

	const Int64 x_eten
		= (Int64)x0_en.x*s_et
		+ (Int64)x1_en.x*t_et
		;

	const Int64 y_eten
		= (Int64)x0_en.y*s_et
		+ (Int64)x1_en.y*t_et
		;

	return int32x2 (x_eten>>et , y_eten>>et);
}

/** specialization: Bezier2.
 * Fast specialization of Bezier2 direct calculation.
 * 6 multiplications 32x32 -> 64 + additions + shifts.
 * UNTESTED.
 * @param e vector components fixed point position
 * @param et time fixed point position (1..32)
 * @param x0_en Bezier control point
 * @param x1_en Bezier control point
 * @param x2_en Bezier control point
 * @param t_et evaluation time, fixed point. This value may be outside of [0,1] in this function, if et<32.
 */
inline static Int32x2 int32x2Bezier2ValueAt_een (int e, int et, const Int32x2 x0_en, const Int32x2 x1_en, const Int32x2 x2_en, const Int32 t_et) {
	const Int32 s2_et = (Int64)((1<<e)-t_et) * ((1<<e)-t_et) >> et;
	const Int32 st_et = (Int64)((1<<e)-t_et) * ( t_et) >> et-1;	// coefficient = 2
	const Int32 t2_et = (Int64)( t_et) * ( t_et) >> et;	

	const Int64 x_eten
		= (Int64)x0_en.x*s2_et
		+ (Int64)x1_en.x*st_et		// coefficient 2 already included above!
		+ (Int64)x2_en.x*t2_et
		;

	const Int64 y_eten
		= (Int64)x0_en.y*s2_et
		+ (Int64)x1_en.y*st_et		// coefficient 2 already included above!
		+ (Int64)x2_en.y*t2_et
		;

	return int32x2 (x_eten>>et , y_eten>>et);
}

/** specialization: Bezier4.
 * Fast specialization of Bezier4 direct calculation.
 * 8 + 5 multiplications 32x32 -> 64 + 1 multiplication 32x32 -> 32 + additions + shifts
 * TESTED: 10/2016
inline static Int32 int32Bezier4ValueAt (const Int32 *xs, const Uint32 t_e32) {
	// const Uint32 s_e32 = -t_e32;	// trivial
	const Uint32 s2_e32 = (Uint64)(-t_e32) * (-t_e32) >> 32;
	const Uint32 t2_e32 = (Uint64)( t_e32) * ( t_e32) >> 32;
	const Uint32 st_e32 = (Uint64)(-t_e32) * ( t_e32) >> 32;	// <= 1/4
	const Uint32 s4_e32 = (Uint64)s2_e32 * s2_e32 >> 32;
	const Uint32 s3t_e32 = (Uint64)s2_e32 * st_e32 >> 30;		// coefficient is 4
	const Uint32 s2t2_e32 = (Uint64)(3*st_e32) * st_e32 >> 31;	// coefficient is 6 = 3*2
	const Uint32 st3_e32 = (Uint64)st_e32 * t2_e32 >> 30;		// coefficient is 4
	const Uint32 t4_e32 = (Uint64)t2_e32 * t2_e32 >> 32;
	const Int64 x_e64
		= (Int64)xs[0]*s4_e32
		+ (Int64)xs[1]*s3t_e32
		+ (Int64)xs[2]*s2t2_e32
		+ (Int64)xs[3]*st3_e32
		+ (Int64)xs[4]*t4_e32
		;
	return x_e64 >> 32;
}
*/

#endif

