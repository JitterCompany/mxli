/*
  int32Math.h - 32bit math that uses 32bit instructions on 32bit CPUs.
  Copyright 2012-2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __int32Math_h
#define __int32Math_h

/** @file
 * @brief Math functions using 32bit integer instructions only.
 */

#include <stdbool.h>
#include <integers.h>

/** Some useful irrational number approximations with leftmost fixed point position possible in unsigned 32-bit integers.
 */
enum Irrational_e30 {
	PI_E30		= 0xc90fdaa2llu,
	PIx2_E29	= PI_E30,
	E_E30		= 0xadf85459llu,
	SQRT2_E31	= 0xb504f334llu,
	SQRT3_E31	= 0xddb3d743llu,
	SQRT2_HALF_E32	= 0xb504f334llu,
	SQRT3_HALF_E32	= 0xddb3d743llu,
};

/** Absolute value as unsigned integer. Works even for INT_MIN
 */
inline static Uint32 int32Abs(Int32 v) {
	return v>=0 ? v : -v;
}

inline static Int32 int32Max(Int32 a, Int32 b) {
	return a>b ? a : b;
}

inline static Int32 int32Min(Int32 a, Int32 b) {
	return a<b ? a : b;
}

inline static Uint32 uint32Max(Uint32 a, Uint32 b) {
	return a>b ? a : b;
}

inline static Uint32 uint32Min(Uint32 a, Uint32 b) {
	return a<b ? a : b;
}

inline static Int32 int32Limit (Int32 min, Int32 max, Int32 value) {
	return value < min ? min : (value > max ? max : value);
}

inline static Uint32 uint32Limit (Uint32 min, Uint32 max, Uint32 value) {
	return value < min ? min : (value > max ? max : value);
}

inline static Int32 int32Avg (Int32 a, Int32 b) {
	return (a+b)/2;
}

inline static Uint32 uint32Avg (Uint32 a, Uint32 b) {
	return (a+b)/2;
}

inline static Int32 int32Sign (Int32 x) {
	return x>0 ? 1 : (x<0 ? -1 : 0);
}

inline static Int32 int64Sign (Int64 x) {
	return x>0 ? 1 : (x<0 ? -1 : 0);
}

/** Mathematical exact DIV operation, that rounds down to -infinity even for negative values.
 * The remainder is always non-negative.
 * @param a the dividend.
 * @param b the divisor. If this is negative, then the sign of dividend and divisor are flipped.
 * @return the quotient of a/b rounded towards -infinity.
 */
Int32 int32MathDiv(Int32 a, Int32 b);

/** Mathematical exact DIV operation, that rounds down to -infinity even for negative values.
 * @param a the dividend.
 * @param b the divisor. If this is negative, then the sign of dividend and divisor are flipped.
 * @return the remainder of the integer division. This is always in the range 0..abs(b)-1 .
 */
Int32 int32MathMod(Int32 a, Int32 b);

/** Maps a value to the closest 'more discrete' value.
 * @param step the increment of the discrete steps. 0 is the first value, 0+step the second and so on.
 * @param value the value to quantize. May be positive or negative.
 * @return that value n*step that is closest to value.
 */
Int32 int32Quantize(Int32 step, Int32 value);

/** Similar to int32Quantize, except that the step value must be a power of 2.
 * @param log2Step the increment of the discrete steps. 0 is the first value, 0+(1<<logStep) the second and so on.
 * @param value the value to quantize. May be positive or negative.
 * @return that value n*(1<<logStep) that is closest to value.
 */
Int32 int32QuantizeExp2(Int32 log2Step, Int32 value);

Uint32 uint32Power(Uint32 x, Uint32 y);

/** Reverses the order of the lower bits of an integer.
 * @param value the bit pattern to reverse
 * @param length the number of bits to reverse.
 * @return a pattern where bits 0..length-1 are mapped to bits length-1..0.
 */
Uint32 uint32BitReverse(Uint32 value, Uint32 length);

/** Rounds correctly at +0.5 even if the values are negative.
 * @param value the value representation to round. The real value will be value/divider.
 * @param divider the divider used to get the real value.
 * @return a rounded representation, that after dividing will yield the correctly rounded result.
 */
inline static Int32 int32RoundForDivide(Int32 value, Int32 divider) {
	return value + (value>=0 ? divider>>1 : -divider>>1);
}

/** Linear interpolation between two points. Only 32-bit calculations are used for
 * improved performance which limits the values. The formula used is
 *
 * r = ( (x1-x)*y0 + (x-x0)*y1 + (x1-x0)/2 ) / (x1-x0)
 *
 * None of the values might be out of the range -2^31..+2^31-1.
 * Interpolation results lie between the two extremes, no extrapolation is performed.
 */
Int32 int32LinearInterpolateFast(Int32 x0, Int32 y0, Int32 x1, Int32 y1, Int32 x);

/** Checks, if int32LinearInterpolateFast() will run with no overflow.
 */
bool int32CanLinearInterpolateFast (Int32 x0, Int32 y0, Int32 x1, Int32 y1);

/** Linear interpolation between two points.
 * Interpolation results lie between the two extremes if x lies between the extremes. Extrapolation
 * is performed. Care must be taken not to overflow the integer ranges, calculations are performed
 * using 32-bit integers only but products of (xi-xj) and ys are summed up. Keep (x1-x0) small and x
 * between these values or keep ys small.
 */
Int32 int32LinearExtrapolateFast(Int32 x0, Int32 y0, Int32 x1, Int32 y1, Int32 x);

/** Fixed point square.
 * @param e the fixed point position
 * @param x_en signed fixed point value
 * @return the square of the fixed point value, always non-negative. No overflow handling.
 */
inline static Uint32 int32Square_en (int e, Int32 x_en) {
	return (Int64)x_en * x_en >> e;
}

/** Fixed point square.
 * @param e the fixed point position
 * @param x_en unsigned fixed point value
 * @return the square of the fixed point value. No overflow handling.
 */
inline static Uint32 uint32Square_en (int e, Uint32 x_en) {
	return (Uint64)x_en * x_en >> e;
}

/** Integer square root, rounded towards 0.
 * @param x the radicant
 * @return the largest integer r such, that r*r <= x
 */
Uint16 uint32SqrtFloor(Uint32 x);

/** Integer square root, rounded towards 0.
 * @param x the radicant
 * @return the smallest integer r such, that r*r >= x. 0xFFFF_FFFF yields 0x10000_0000, which doesn't fit into an
 *   Uint16, hence the different return type.
 */
inline static Uint32 uint32SqrtCeil(Uint32 x) {
	const Uint32 r = uint32SqrtFloor(x);	// r<=2^16-1
	if (r*r>=x) return r; else return r+1;	// r<=2^16
}

/** Fixed point square root. This function uses 32-bit math only resulting in 16 significant bits in the result.
 * 64-bit math is avoided at the cost of precision loss.
 * @param e position of the (binary) point. e bits to the right are fractional ones.
 * @param x_en the radicant. e bits to the right are fractional ones.
 * @return fixed point square root approximation r_en with r_en*r_en<=x_en with binary point at position e .
 */
Uint32 uint32SqrtFloorFast_en (int e, Uint32 x_en);

/** Calculates the integral part log2, rounded up.
 * @param x a value >=1, 0 is allowed as an exception.
 * @return the smallest value l such that 2^l >= x. This value is in the range of 0..32 (yes, 32).
 * If x==0, then -32 is returned.
 */
inline static int uint32Log2Ceil(Uint32 x) {
	if (x==0) return -32;
	for (int i=0; i<32; ++i) if (1<<i >= x) return i;
	return 32;
}

/** Calculates the integral part log2, rounded down.
 * @param x a value >= 1.
 * @return the biggest value l such that 2^l <= x . This value is in the range of 0..31.
 * If x==0 then -32 is returned (good approximation of 0 for Int32).
 */
inline static int uint32Log2Floor(Uint32 x) {
	for (int l=31; l>=0; --l) if (1<<l <= x) return l;
	return -32;	// approximation of 0
}

/** Shift function allowing positive and negative counts.
 * @param a the value to shift
 * @param p the number of bits to shift, positive or negative.
 * @return the signed shift result.
 */
static inline Int32 int32MulExp2(Int32 a, int p) {
	return (p>=0) ? a<<p : a>>-p;
}

/** Shift function allowing positive and negative counts.
 * @param a the value to shift
 * @param p the number of bits to shift, positive or negative.
 * @return the signed shift result.
 */
static inline Uint32 uint32MulExp2(Uint32 a, int p) {
	if (p>=0) return a << p;
	else return a >> -p;
}

/** Good approximation of 2^32 / n without using 64 bit calculations.
 * @param n divisor greater than 0
 * @return a number x such that n*x <= 2^32, with best approximation of division.
 */
inline static Uint32 uint32E32DivN (Uint32 n) {
	return (n&1) ? ~0u/n : (1u<<31)/(n/2);
}

/** Calculation of 2^32 / n without using 64 bit calculations. Rounding towards infinity. Less precision
 * than uint32E32DivN() because I can't do the all-ones trick here.
 * BUGS: the result is very unprecise for small numbers n. Needs re-working (theoretically difficult)
 * @param n divisor greater than 1 (no, one is not allowed, because the result doesn't fit into Uint32).
 * @return a number x such that n*x >= 2^32
 */
inline static Uint32 uint32E32DivNCeil (Uint32 n) {
	const Uint32 f_e32 = uint32E32DivN (n);		// for even n this may be an exact result
	return 0==(Uint32)(f_e32*n) ? f_e32 : f_e32+1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// simple range iterator

/** Fast Iterator over the range 0..2^32-1 using 32-bit math, only.
 */
struct Uint32RangeIterator {
	Uint32	dt;
	Uint32	t;
};
typedef struct Uint32RangeIterator Uint32RangeIterator;

/** Sets the iterator to zero and remembers the step size.
 * @param it the iterator
 * @param dt_e32 the 32-bit step size, interpreted as the value dt/2^32 < 1.
 */
inline static void uint32RangeIteratorStartStep (Uint32RangeIterator * it, Uint32 dt_e32) {
	it->t = 0;
	it->dt = dt_e32;
}

/** Sets the iterator to zero and remembers the step size.
 * BUGS: the final step before overflow can be VERY close to 1.
 * @param it the iterator
 * @param steps the number of steps from 0 to '1'.
 */
inline static void uint32RangeIteratorStartN (Uint32RangeIterator * it, Uint32 steps) {
	it->t = 0;
	it->dt = uint32E32DivNCeil (steps);
}

/** Returns the current (32-bit) value of the iterator.
 * @param it the iterator
 * @return current value. This value may be the same (!) even after several iterations, if the step size is pretty
 *   small.
 */
inline static Uint32 uint32RangeIteratorValue (const Uint32RangeIterator * it) {
	return it->t;
}

/** Advances by one step. Please note, that the corresponding value (32-bit, truncated) may not change neccessarily.
 * The only valid criterion for iterator termination is the return value of this function.
 * @param it the iterator
 * @return true, if iterator advanced, false if end of range is reached (and value remains unchanged).
 */
inline static bool uint32RangeIteratorNext (Uint32RangeIterator * it) {
	Uint32 tNext = it->t + it->dt;
	if (tNext>=it->t) {
		it->t = tNext;
		return true;
	}
	else return false;
}

#endif
