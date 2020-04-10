/*
  bezierN.h 
  Copyright 2016 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */
#ifndef __bezierN_h
#define __bezierN_h

/** @file
 * @brief Bezier curves of order 4 with fixed point calculation only (64bit, 32bit results).
 */

#include <stdbool.h>
#include <integers.h>
#include <int64Math.h>
#include <fifoPrintFixedPoint.h>
#include <bezier.h>

typedef Int32 Int32Poly0 [1];		// a,				lowest power = first element
typedef Uint32 Uint32Powers0_e32 [0];	// 0				lowest power = first element
typedef Int32Poly0 Int32Bezier0;
typedef Uint32 Uint32BezierPowers0_e32 [1];

typedef Int32 Int32Poly1 [2];		// a + b*x,			lowest power = first element
typedef Uint32 Uint32Powers1_e32 [1];	// x,				lowest power = first element
typedef Int32Poly1 Int32Bezier1;
typedef Uint32 Uint32BezierPowers1_e32 [2];

//typedef Int32 Int32Poly2 [3];		// a + b*x + c*x^2,		lowest power = first element
typedef Uint32 Uint32Powers2_e32 [2];	// x,x^2,			lowest power = first element
typedef Int32Poly2 Int32Bezier2;
typedef Uint32 Uint32BezierPowers2_e32 [3];

//typedef Int32 Int32Poly3 [4];		// a + b*x + c*x^2 + d*x^3,	lowest power = first element
typedef Uint32 Uint32Powers3_e32 [3];	// x,x^2,x^3,			lowest power = first element
typedef Int32Poly3 Int32Bezier3;
typedef Uint32 Uint32BezierPowers3_e32 [4];

typedef Int32 Int32Poly4 [5];		// a + b*x + c*x^2 + d*x^4,	lowest power = first element
typedef Uint32 Uint32Powers4_e32 [4];	// x,x^2,x^3,x^4		lowest power = first element
typedef Int32Poly4 Int32Bezier4;
typedef Uint32 Uint32BezierPowers4_e32 [5];

typedef Int32 Int32Poly5 [6];		// a + b*x + c*x^2 + d*x^5,	lowest power = first element
typedef Uint32 Uint32Powers5_e32 [5];	// x,x^2,..,x^5			lowest power = first element
typedef Int32Poly5 Int32Bezier5;
typedef Uint32 Uint32BezierPowers5_e32 [6];

typedef Int32 Int32Poly6 [7];		// a + b*x + .. + d*x^6,	lowest power = first element
typedef Uint32 Uint32Powers6_e32 [6];	// x,x^2,x^3,x^6		lowest power = first element
typedef Int32Poly6 Int32Bezier6;
typedef Uint32 Uint32BezierPowers6_e32 [7];

typedef Int32 Int32Poly7 [8];		// a + b*x + .. + d*x^7,	lowest power = first element
typedef Uint32 Uint32Powers7_e32 [7];	// x,x^2,..,x^7			lowest power = first element
typedef Int32Poly7 Int32Bezier7;
typedef Uint32 Uint32BezierPowers7_e32 [8];

typedef Int32 Int32Poly8 [9];		// a + b*x + .. + d*x^8,	lowest power = first element
typedef Uint32 Uint32Powers8_e32 [8];	// x,x^2,..,x^8			lowest power = first element
typedef Int32Poly8 Int32Bezier8;
typedef Uint32 Uint32BezierPowers8_e32 [9];

typedef Int32 Int32Poly9 [10];		// a + b*x + .. + d*x^9		lowest power = first element
typedef Uint32 Uint32Powers9_e32 [9];	// x,x^2,..,x^9			lowest power = first element
typedef Int32Poly9 Int32Bezier9;
typedef Uint32 Uint32BezierPowers9_e32 [10];

////////////////////////////////////////////////////////////////////////////////////////////////////
// Printing

/** Convenience function for assignment one-liners.
 */
inline static void int32Bezier4Set (Int32Bezier4 xs, Int32 x0, Int32 x1, Int32 x2, Int32 x3, Int32 x4) {
	xs[0] = x0;
	xs[1] = x1;
	xs[2] = x2;
	xs[3] = x3;
	xs[4] = x4;
}

inline static bool fifoPrintInt32Poly_e20 (Fifo *o, int n, const Int32 *poly, int minWidth, int precision) {
	fifoPrintString (o,"(Int32Poly");
	fifoPrintInt (o,n);
	fifoPrintString (o,"){");
	for (int i=0; i<=n; i++) {
		if (poly[i]>=0) fifoPrintChar (o,'+');
		fifoPrintInt32_e20 (o,poly[i],minWidth,precision);
		if (i>=1) fifoPrintString (o,"*t");
		if (i>=2) {
			fifoPrintChar (o,'^');
			fifoPrintInt (o,i);
		}
	}
	return fifoPrintChar (o,'}');
}

inline static bool fifoPrintInt32Bezier_e20 (Fifo *o, int n, const Int32 *poly, int minWidth, int precision) {
	fifoPrintString (o,"(Int32Bezier");
	fifoPrintInt (o,n);
	fifoPrintString (o,"){");
	for (int i=0; i<=n; i++) {
		if (i>=1) fifoPrintChar (o,',');
		fifoPrintInt32_e20 (o,poly[i],minWidth,precision);
	}
	return fifoPrintChar (o,'}');
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// n-th order Bezier Iterator
//

/** Recursive function. To avoid stack dangers, n is limited to 10 (current maximum value required).
 */
inline static Int32 binomial (int n, int k) {
	enum { N_MAX = 10, };

	if (k==0) return 1;
	else if (k==1) return n;
	else if (2*k>n) return binomial (n,n-k);
	else return n<=N_MAX ? binomial (n-1,k-1) + binomial (n-1,k) : 0;
}

/** Calculates the powers of t.
 * @param n the highest power
 * @param t_e32 the t^1 value
 * @param powers a n-elements array of the powers. Value '1' cannot be represented (overflow).
 */
inline static void uint32Powers_e32 (int n, Uint32 t_e32, Uint32 *powers) {
	powers[0] = t_e32;
	for (int i=1; i<n; i++) powers[i] = (Uint64)t_e32 * powers[i-1] >> 32;
}

/** Calculates the powers of the next step.
 * @param n the highest power
 * @param step_e32 the t^1 increment
 * @param powers a n-elements array of the powers. Value '1' cannot be represented (overflow).
 */
inline static bool uint32PowersNext_e32 (int n, Uint32 step_e32, Uint32 *powers) {
	Uint32 new_e32 = powers[0] + step_e32;
	if (new_e32>powers[0]) {		// no wraparound/overflow
		uint32Powers_e32 (n,new_e32,powers);
		return true;
	}
	else return false;
}

/** Resets all powers to 0.
 * @param n highest power
 * @param powers an n-element array of values.
 */
inline static void uint32PowersStart_e32 (int n, Uint32 *powers) {
	for (int i=0; i<n; i++) powers[i] = 0;
}

/** Calculates the Bezier weights B(n,k)(t) = binomial(n,k)*(t-1)^(n-k)*t^k for any value 0 < t < 1.
 * The extremes won't be exact.
 */
inline static void uint32BezierWeights_e32 (int n, Uint32 t_e32, Uint32 *weights_e32) {
	Uint32 tn_e32 = t_e32;
	for (int i=1; i<=n; i++) {
		weights_e32[i] = tn_e32;
		tn_e32 *= t_e32;
	}
	tn_e32 = -t_e32;	// = 1-t
	for (int i=n-1; i>=0; i--) {
		weights_e32[i] *= tn_e32 * binomial (n,n-i);
		tn_e32 *= -t_e32;
	}
}

/** specialization: Bezier4.
 * Fast specialization of Bezier4 direct calculation.
 * 8 + 5 multiplications 32x32 -> 64 + 1 multiplication 32x32 -> 32 + additions + shifts
 * TESTED: 10/2016
 */
inline static void uint32Bezier4Weights_e32 (const Uint32 t_e32, Uint32 *weights_e32) {
	// const Uint32 s_e32 = -t_e32;	// trivial
	const Uint32 s2_e32 = (Uint64)(-t_e32) * (-t_e32) >> 32;
	const Uint32 t2_e32 = (Uint64)( t_e32) * ( t_e32) >> 32;
	const Uint32 st_e32 = (Uint64)(-t_e32) * ( t_e32) >> 32;	// <= 1/4
	weights_e32[0] = (Uint64)s2_e32 * s2_e32 >> 32;
	weights_e32[1] = (Uint64)s2_e32 * st_e32 >> 30;		// coefficient is 4
	weights_e32[2] = (Uint64)(3*st_e32) * st_e32 >> 31;	// coefficient is 6 = 3*2
	weights_e32[3] = (Uint64)st_e32 * t2_e32 >> 30;		// coefficient is 4
	weights_e32[4] = (Uint64)t2_e32 * t2_e32 >> 32;
}

/** Direct Bezier evaluation. Numerically much better than a polynome with its subtractions.
 * @param n highest power
 * @param bezier the curve defining points x0..xn
 * @param weights_e32 the Bernstein polynome B(n,v)(t) values.
 * @return the curve point at position t.
 */
inline static Int32 int32BezierValue_e32 (int n, const Int32 *bezier, const Uint32 *weights_e32) {
	Uint64 sum_e32 = 0;
	for (int i=0; i<=n; i++) sum_e32 += (Uint64)bezier[i] * weights_e32[i];
	return sum_e32 >> 32;
}

/** Evaluates polynome x(t) at current value of powers iterator.
 */
inline static Int32 int32PolyValue_e32 (int n, const Int32 *poly, const Uint32 *powers) {
	Int64 sum_e64 = (Int64)poly[0]<<32;
	for (int i=1; i<=n; i++) sum_e64 += poly[i]*(Int64)powers[i-1];
	return sum_e64 >> 32;
}

/** Evaluation of polynome using rule of Ruffini (published it 15 years before 'Horner').
 */
inline static Int32 int32PolyRuffiniValue_e32 (int n, const Int32 *poly, Uint32 t_e32) {
	Int32 mla = poly[n];
	for (int i=n-1; i>=0; i--) mla = poly[i] + (mla * (Int64)t_e32 >> 32);
	return mla;
}

inline static Int32 int32PolyValueAtZero_e32 (int n, const Int32 *poly) {
	return poly[0];
}

/** Evaluates polynome at value t=1, which cannot be represented by the powers iterator.
 */
inline static Int32 int32PolyValueAtOne_e32 (int n, const Int32 *poly) {
	Int32 sum_e32 = 0;
	for (int i=0; i<=n; i++) sum_e32 += poly[i];
	return sum_e32;
}

/** Evaluates polynome 'speed' d/dt x(t) at current value of powers iterator.
 */
inline static Int32 int32PolySpeed_e32 (int n, const Int32 *poly, const Uint32 *powers) {
	Int64 sum_e64 = (Int64)poly[1]<<32;
	for (int i=2; i<=n; i++) sum_e64 += poly[i]*(Int64)powers[i-2]*i;
	return sum_e64 >> 32;
}

/** Evaluates polynome 'speed' d/dt x(t) at t=0.
 */
inline static Int32 int32PolySpeedAtZero_e32 (int n, const Int32 *poly) {
	return poly[1];
}

/** Evaluates polynome 'speed' d/dt x(t) at t=1.
 */
inline static Int32 int32PolySpeedAtOne_e32 (int n, const Int32 *poly) {
	Int32 sum_e32 = 0;
	for (int i=1; i<=n; i++) sum_e32 += poly[i]*i;
	return sum_e32;
}

/** Evaluates polynome 'acceleration' d^2/dt^2 x(t) at current value of powers iterator.
 */
inline static Int32 int32PolyAcceleration_e32 (int n, const Int32 *poly, const Uint32 *powers) {
	Int64 sum_e64 = 1*2*(Int64)poly[2]<<32;
	for (int i=3; i<=n; i++) sum_e64 += poly[i]*(Int64)powers[i-3]*i*(i-1);
	return sum_e64 >> 32;
}

/** Evaluates polynome 'acceleration' d^2/dt^2 x(t) at t=0.
 */
inline static Int32 int32PolyAccelerationAtZero_e32 (int n, const Int32 *poly) {
	return 1*2*poly[2];
}

/** Evaluates polynome 'acceleration' d^2/dt^2 x(t) at t=1.
 */
inline static Int32 int32PolyAccelerationAtOne_e32 (int n, const Int32 *poly) {
	Int32 sum_e32 = 1*2*poly[2];
	for (int i=3; i<=n; i++) sum_e32 += poly[i]*i*(i-1);
	return sum_e32;
}


inline static int minus1Exp (int i) {
	return i&1 ? -1 : 1;
}

/** Plain polynome coefficients from Bezier defining points
 */
inline static Int32 int32BezierToPolyCoefficient (int n, const Int32 *xs, Int32 power) {
	Int32 a = 0;
	for (int i=0; i<=power; i++) a += minus1Exp(power+i) * binomial(power,i) * xs[i];
	return a;
}

/** Polynome coefficients from Bezier defining points. Take care to provide correct sizes of vectors!
 */
inline static void int32BezierToPoly (int n, const Int32 *xs, Int32* poly) {
	for (int p=0; p<=n; p++) {
		poly[p] = 0;
		for (int i=0; i<=p; i++) poly[p] += minus1Exp(p+i) * binomial(p,i) * xs[i];
		poly[p] *= binomial (n,p);
	}
}

/** Calculates a Bezier polynome value from the pre-calculated powers t^n, (1-t)^1. This is done without an
 * intermediate converstion into a polynome in t (only). Use for infrequent calculations, only!
 * @param n the order of the Bezier polynome
 * @param xs the defining points of the Bezier polynome
 * @param t1n the powers 1..n of (1-t)
 * @param tn the powers 1..n of t
 * @return the curve point at time t in [0,1)
 */
inline static Int32 int32BezierValue (int n, const Int32 *xs, const Uint32 *t1n, const Uint32 *tn) {
	if (tn[0]==0) return xs[0];
	else {
		Int64 sum_e64 = 0;
		for (int i=0; i<=n; i++) {
			sum_e64 +=
				( ((i<=n-1) ? (Int64)t1n[n-1-i] : 1llu<<32 ) * ( i>=1 ? (Int64)tn[i-1] : 1llu<<32) >> 32
				)
				* binomial (n,i) * xs[i];
		}
		return sum_e64>>32;
	}
}

/** Calculates the powers t^n and (1-t)^n.
 */
inline static Int32 int32BezierValueAt (int n, const Int32 *xs, const Uint32 t_e32) {
	if (n<=0) return 0;
	if (t_e32==0) return xs[0];

	Uint32 powersT_e32 [n];
	Uint32 powers1mT_e32 [n];
	for (int i=0; i<n; i++) {
		powersT_e32[i] = (i>=1) ? (Int64)powersT_e32[i-1] * t_e32 >> 32 : t_e32;
		powers1mT_e32[i] = (i>=1) ? (Int64)powers1mT_e32[i-1] * -t_e32 >> 32 : -t_e32;
	}
	return int32BezierValue (n,xs,powers1mT_e32,powersT_e32);
}

inline static Int32 int32BezierValueAt0 (int n, const Int32 *xs) {
	return xs[0];
}

/** The bezier curve value at t=0.5 . This one is exceptionally easy to calculate, as (1-t)^i * t^(n-1) = 1/2^n at this
 * point.
 */
inline static Int32 int32BezierValueAt05 (int n, const Int32 *xs) {
	Int64 sum;
	for (int i=0; i<=n; i++) sum += (Int64)binomial(n,i)*xs[i];
	return sum>>n;
}

inline static Int32 int32BezierValueAt1 (int n, const Int32 *xs) {
	return xs[n];
}

/** specialization: Bezier2.
 * Fast specialization of Bezier2 direct calculation.
 * 6 multiplications 32x32 -> 64 + additions + shifts.
 * TESTED: 10/2016
 */
inline static Int32 int32Bezier2ValueAt (const Int32 *xs, const Uint32 t_e32) {
	// const Uint32 s_e32 = -t_e32;	// trivial
	const Uint32 s2_e32 = (Uint64)(-t_e32) * (-t_e32) >> 32;
	const Uint32 st_e32 = (Uint64)(-t_e32) * ( t_e32) >> 31;	// coefficient = 2
	const Uint32 t2_e32 = (Uint64)( t_e32) * ( t_e32) >> 32;	

	const Int64 x_e64
		= (Int64)xs[0]*s2_e32
		+ (Int64)xs[1]*st_e32		// coefficient 2 already included above!
		+ (Int64)xs[2]*t2_e32
		;
	return x_e64 >> 32;
}

/** specialization: Bezier4.
 * Fast specialization of Bezier4 direct calculation.
 * 8 + 5 multiplications 32x32 -> 64 + 1 multiplication 32x32 -> 32 + additions + shifts
 * TESTED: 10/2016
 */
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

inline static Int32 int32BezierSpeedAt0 (int n, const Int32 *xs) {
	return n>=1 ? n*(xs[1]-xs[0]) : 0;
}

inline static Int32 int32BezierSpeedAt1 (int n, const Int32 *xs) {
	return n>=1 ? n*(xs[n]-xs[n-1]) : 0;
}


inline static Int32 int32BezierAccelerationAt0 (int n, const Int32 *xs) {
	return n>=2 ? -xs[0]+2*xs[1]-xs[2] : 0;
}

inline static Int32 int32BezierAccelerationAt1 (int n, const Int32 *xs) {
	return n>=2 ? -xs[n-2]+2*xs[n-1]-xs[n] : 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Special functions
//

/** This function generates an order 4 polynome for a Bezier order 2 curve, with choice of starting and
 * end point speeds. Highest intermediate factor: 6, i.e. x-range = (2^31-1) / 6 = 357.9*10^6.
 * The higher speed is up to 2x the Bezier2 speed, the lower speed may be down to 0.
 * @param xs2 3 points of Bezier2 definition
 * @param xs4 destination Bezier4 array (5 elements).
 * @param alpha_e30 acceleration towards end point in the range from -1 to +1 (-E30..+E30).
 * TESTED: 10/2016
 */
inline static void int32Bezier2ToBezier4 (const Int32 *xs2, Int32* xs4, Int32 alpha_e30) {
	const Uint32 OnePlusAlpha_e30 = alpha_e30 >= -1<<30
		? (1u<<30) + alpha_e30	// range: 0..E31, fits into a 32-bit ** unsigned **
		: 0;	// heuristic: clamp to zero, if alpha is slightly outside valid range
	const Uint32 OneMinusAlpha_e30 = alpha_e30 <= 1<<30
		? (1u<<30) - alpha_e30	// range: 0..E31, fits into a 32-bit ** unsigned **
		: 0;	// heuristic: clamp to zero, if alpha is slightly outside valid range
	xs4[0] = xs2[0];
	xs4[1] = (	(Int64)xs2[0] * OnePlusAlpha_e30 + (Int64)xs2[1] * OneMinusAlpha_e30) / 2 >> 30 ;
	xs4[2] = int64Div (
		(	(Int64)xs2[0] *( (Int64)OnePlusAlpha_e30*OnePlusAlpha_e30 >>30 ) +			// max 4*E30
			(Int64)xs2[1] *2* ((1ll<<30) + ((Int64)OnePlusAlpha_e30*OneMinusAlpha_e30 >>30)) +	// max 2*4*E30
			(Int64)xs2[2] *( (Int64)OneMinusAlpha_e30*OneMinusAlpha_e30  >>30)			// max 4*E30
		) ,6 ) >> 30 ; // sum: max 6*E30 
	xs4[3] = (	(Int64)xs2[1] * OnePlusAlpha_e30 + (Int64)xs2[2] * OneMinusAlpha_e30) / 2 >> 30 ;
	xs4[4] = xs2[2] ;
}

/** This function generates an order 4 polynome for a Bezier order 2 curve, with choice of starting and
 * end point speeds. Highest intermediate factor: 6, i.e. x-range = (2^31-1) / 6 = 357.9*10^6.
 * The higher speed is up to 2x the Bezier2 speed, the lower speed may be down to 0.
 * @param xs2 3 points of Bezier2 definition
 * @param poly4 polynome array (5 elements) for polynome of degree 4, normally resulting from a Bezier4 definition.
 * @param alpha_e30 acceleration towards end point in the range from -1 to +1 (-E30..+E30).
 */
inline static void int32Bezier2ToPoly4 (const Int32 *xs2, Int32* poly4, Int32 alpha_e30) {
	Int32Bezier4 xs4;
	int32Bezier2ToBezier4 (xs2,xs4,alpha_e30);
	int32BezierToPoly (4,xs4,poly4);
}

/** Splits a bezier curve of order 2 into 2 segments.
 */
inline static void int32Bezier2Split (const Int32 *xs, Int32 *ys, Int32 *zs) {
	ys[0] = xs[0];
	zs[2] = xs[2];

	const Int64 xm = ( (Int64)xs[0] + 2ll*xs[1] + xs[2] ) / 4;	// t=0.5 value of curve
	ys[2] = xm;
	zs[0] = xm;

	ys[1] = ((Int64) xs[0] + xs[1]) / 2;
	zs[1] = ((Int64) xs[1] + xs[2]) / 2;
}

/** Tangential speed of the curve at t=0.5.
 */
inline static Int32 int32Bezier2SpeedAt05 (const Int32 *xs) {
	return xs[2]-xs[0];	// that simple, really!
}

#endif

