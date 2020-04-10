/*
  bezier.h 
  Copyright 2016 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */
#ifndef __bezier_h
#define __bezier_h

/** @file
 * @brief Square/Cubic Bezier curves with fixed point calculation only (64bit, 32bit results).
 */

#include <stdbool.h>
#include <integers.h>

typedef Int32 Int32Poly2 [3];	// a + b*x + c*x^2,		lowest power = first element
typedef Int32 Int32Poly3 [4];	// a + b*x + c*x^2 + d*x^3,	lowest power = first element

////////////////////////////////////////////////////////////////////////////////////////////////////
// Generic functionality

/** Calculates the (global extreme) of the (1-dimensional) bezier curve.
 * @param x0 starting point
 * @param x1 tangents-defining point
 * @param x2 final point
 * @return the extreme value (minimum or maximum) of the (possibly extrapolated beyond t=[0,1]) curve.
 */
Int32 int32Bezier2GlobalExtreme (Int32 x0, Int32 x1, Int32 x2);

/** Calculates the smallest value (towards negative infinity) of the curve between x0 and x2.
 * @param x0 starting point
 * @param x1 tangents-defining point
 * @param x2 final point
 * @return value minimum
 */
Int32 int32Bezier2Min (Int32 x0, Int32 x1, Int32 x2);

/** Calculates the greatest value (towards positive infinity) of the curve between x0 and x2.
 * @param x0 starting point
 * @param x1 tangents-defining point
 * @param x2 final point
 * @return value maximum
 */
Int32 int32Bezier2Max (Int32 x0, Int32 x1, Int32 x2);

////////////////////////////////////////////////////////////////////////////////////////////////////
// 2. Order Bezier Iterator
//
/** Calculates the powers of delta-t. The calculation has errors at the final point, unless tStep_e32 is E32>>k.
 */
struct SquareIterator {
	Uint32		tStep_e32;	///< this had better be an integral fraction of E32!

	Uint32		t_e32;
	Uint32		t2_e32;
};

typedef struct SquareIterator SquareIterator;

inline static Int32 squareIteratorT_e32 (const SquareIterator *it) {
	return it->t_e32;
}

inline static Int32 squareIteratorT2_e32 (const SquareIterator *it) {
	return it->t2_e32;
}


inline static void squareIteratorStart (SquareIterator *i, Uint32 tStep_e32) {
	i->tStep_e32 = tStep_e32;
	i->t_e32 = 0;
	i->t2_e32 = 0;
}

/** This version uses a 32x32->64 bit multiplication under the assumption, that this operation is fast.
 * @return true, unless final point reached/crossed.
 */
inline static bool squareIteratorNext (SquareIterator *i) {
	Uint32 new = i->t_e32 + i->tStep_e32;
	if (new>i->t_e32) {
		i->t_e32 = new;
		i->t2_e32 = (Uint64)new * new >> 32;
		return true;
	}
	else return false;
}

/** Plain polynome coefficients from Bezier defining points
 */
inline static void int32Bezier2ToPoly2 (Int32 x0, Int32 x1, Int32 x2, Int32 *poly) {
	poly[0] = x0;
	poly[1] = 2*x1 - 2* x0;
	poly[2] = x0 - 2*x1 + x2;
}

/** Linear interpolation, acceleration=0.
 */
inline static void int32Bezier1ToPoly2 (Int32 x0, Int32 x1, Int32 *poly) {
	int32Bezier2ToPoly2 (x0,(x0+x1)/2,x1,poly);
}

/** Linear interpolation, positive acceleration, v=0 in starting point.
 */
inline static void int32Bezier2StartToPoly2 (Int32 x0, Int32 x1, Int32 *poly) {
	int32Bezier2ToPoly2 (x0,x0,x1,poly);
}

/** Linear interpolation, negative acceleration, v=0 in endpoint.
 */
inline static void int32Bezier2StopToPoly2 (Int32 x0, Int32 x1, Int32 *poly) {
	int32Bezier2ToPoly2 (x0,x1,x1,poly);
}


/** Calculates a point on the curve from interator and polynome coeffs.
 */
inline static Int32 int32Poly2IteratorValue (const Int32 *poly, const SquareIterator *it) {
	return	poly[0]
		+ (Int32)((Int64)poly[1] * it->t_e32 >> 32)
		+ (Int32)((Int64)poly[2] * it->t2_e32 >> 32)
		;
}

inline static Int32 int32Poly2AtZero (const Int32 *poly) {
	return poly[0];
}

inline static Int32 int32Poly2AtOne (const Int32 *poly) {
	return poly[0] + poly[1] + poly[2];
}


/** Calculates the speed on the curve from interator and polynome coeffs.
 */
inline static Int32 int32Poly2IteratorSpeed (const Int32 *poly, const SquareIterator *it) {
	return	poly[1]
		+ 2*(Int32)((Int64)poly[2] * it->t_e32 >> 32)
		;
}

/** Calculates the speed on the curve at the starting point t=0.
 */
inline static Int32 int32Poly2IteratorSpeedAtZero (const Int32 *poly) {
	return	poly[1];
}

/** Calculates the speed on the curve at the final point t=1.
 */
inline static Int32 int32Poly2IteratorSpeedAtOne (const Int32 *poly) {
	return	poly[1] + 2*poly[2];
}


/** Calculates the acceleration on the curve from interator and polynome coeffs.
 */
inline static Int32 int32Poly2IteratorAcceleration (const Int32 *poly) {
	return	2*poly[2];
}

/** Calculates the jerk on the curve from interator and polynome coeffs.
 */
inline static Int32 int32Poly2IteratorJerk (void) {
	return	0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// 3. Order Bezier Iterator
//

inline static Uint64 uint64FractionCube (Uint32 e32) {

	if (e32 <= 1<<21) {	// exact result for values less than 2^-11 = 0.00049 or 1/2048
		return (Uint64) e32 * e32 * e32 >> 32;
	}
	else {			// need some truncation; 21 bits remain intact
		e32 >>= 11;
		return (Uint64)e32 * e32 * e32 * 2;
	}
}

/** Iterator using 32x32->64 bit multiplications
 */
struct CubicIterator {
	Uint32		dt_e32;		///< step value
	Uint32		t_e32;		///< linear position
	Uint32		t2_e32;
	Uint32		t3_e32;
};

typedef struct CubicIterator CubicIterator;

inline static void cubicIteratorStart (CubicIterator *it, Uint32 dt_e32) {
	it->dt_e32 = dt_e32;
	it->t_e32 = 0;
	it->t2_e32 = 0;
	it->t3_e32 = 0;
}

bool cubicIteratorNext (CubicIterator *it);
inline static Uint32 cubicIteratorT_e32 (const CubicIterator *it) {
	return it->t_e32;
}

inline static Uint32 cubicIteratorT2_e32 (const CubicIterator *it) {
	return it->t2_e32;
}

inline static Uint32 cubicIteratorT3_e32 (const CubicIterator *it) {
	return it->t3_e32;
}

Int32 int32Poly3CubicIteratorValue (const Int32 *poly, const CubicIterator *it);
Int32 int32Poly3CubicIteratorSpeed (const Int32 *poly, const CubicIterator *it);
Int32 int32Poly3CubicIteratorAcceleration (const Int32 *poly, const CubicIterator *it);
Int32 int32Poly3CubicIteratorJerk (const Int32 *poly);	// constant value

inline static Int32 int32Poly3AtZero (const Int32 *poly) {
	return poly[0];
}

inline static Int32 int32Poly3AtOne (const Int32 *poly) {
	return poly[0] + poly[1] + poly[2] + poly[3];
}

inline static Int32 int32Poly3CubicIteratorSpeedAtZero (const Int32 *poly) {
	return	poly[1];
}

inline static Int32 int32Poly3CubicIteratorSpeedAtOne (const Int32 *poly) {
	return	poly[1] + 2*poly[2] + 3*poly[3];
}

inline static Int32 int32Poly3CubicIteratorAccelerationAtZero (const Int32 *poly) {
	return 2*poly[2];
}

inline static Int32 int32Poly3CubicIteratorAccelerationAtOne (const Int32 *poly) {
	return 2*poly[2] + 6*poly[3];
}

/** Iterator using NO MULTIPLICATIONs, but sums and differences instead.
 */
struct CubicIncrementalIterator {
	Uint32		dt_e32;
	Uint64		dt2_e64;
	Uint64		dt3_e64;

	Uint32		t_e32;
	Uint64		t2_e64[2];	// indices into the past
	Uint64		t3_e64[3];
};

typedef struct CubicIncrementalIterator CubicIncrementalIterator;

/** Starts an iteration 0..1-e, e<=dt
 * Deviations of t^3 are as follows: dt=1E-5(10u)->75u, dt=5E4(20u)->13u, dt=1E6(1u)->2400u
 * @param it the iterator structure, possibly uninitialized
 * @param dt_e32 linear step. Must be in the range E32>>21 .. E32 or better 1E-5*E32 .. E32 
 */
void cubicIncrementalIteratorStart (CubicIncrementalIterator *it, Uint32 dt_e32);

/** Calculates the next step of the iterator along with all powers of it.
 * @return true if the step doesn't yet reach '1', false otherwise (and iterator unmodified).
 */
bool cubicIncrementalIteratorNext (CubicIncrementalIterator *it);


inline static Uint32 cubicIncrementalIteratorT_e32 (const CubicIncrementalIterator *it) {
	return it->t_e32;
}

inline static Uint32 cubicIncrementalIteratorT2_e32 (const CubicIncrementalIterator *it) {
	return it->t2_e64[1] >>32;
}

inline static Uint32 cubicIncrementalIteratorT3_e32 (const CubicIncrementalIterator *it) {
	return it->t3_e64[2] >>32;
}


inline static Uint32 cubicIncrementalIteratorT3_e64 (const CubicIncrementalIterator *it) {
	return it->t3_e64[2];
}


Int32 int32Poly3CubicIncrementalIteratorValue (const Int32 *poly, const CubicIncrementalIterator *it);

/** Linear changing acceleration
 */
void int32Bezier3ToPoly3 (Int32 x0, Int32 x1, Int32 x2, Int32 x3, Int32 *poly);

/** Constant acceleration curve
 */
void int32Bezier2ToPoly3 (Int32 x0, Int32 x1, Int32 x2, Int32 *poly);

/** Linear changing acceleration, speed=0 at x0.
 */
inline static void int32Bezier3StartToPoly3 (Int32 x0, Int32 x1, Int32 x2, Int32 *poly) {
	int32Bezier3ToPoly3 (x0,x0,x1,x2,poly);
}

/** Linear changing acceleration, speed=0 at x2.
 */
inline static void int32Bezier3StopToPoly3 (Int32 x0, Int32 x1, Int32 x2, Int32 *poly) {
	int32Bezier3ToPoly3 (x0,x1,x2,x2,poly);
}

/** Zero acceleration line.
 */
inline static void int32Bezier1ToPoly3 (Int32 x0, Int32 x1, Int32 *poly) {
	int32Bezier2ToPoly3 (x0, (x0+x1)/2, x1, poly);
}

/** Zero acceleration line.
 */
inline static void int32Bezier1StartToPoly3 (Int32 x0, Int32 x1, Int32 *poly) {
	int32Bezier3StartToPoly3 (x0,x0,x1, poly);
}

/** Zero acceleration line.
 */
inline static void int32Bezier1StopToPoly3 (Int32 x0, Int32 x1, Int32 *poly) {
	int32Bezier3StopToPoly3 (x0,x1,x1,poly);
}

#endif
