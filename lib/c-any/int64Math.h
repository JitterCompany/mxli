/*
  int64Math.h - math functions, that tend to be slow on 32bit CPUs.
  Copyright 2012-2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef int64Math_h
#define int64Math_h

#include <stdbool.h>
#include <integers.h>

/** @file
 * @brief 64-bit integer math.
 *
 * This module provides functions, that may be considerably less efficient than the corresponding int32-functions
 * on a 32-bit processor because of emulation of 64-bit calculations.
 */


inline static Int64 int64Abs(Int64 v) {
	return v>=0 ? v : -v;
}

inline static Int64 int64Max(Int64 a, Int64 b) {
	return a>b ? a : b;
}

inline static Int64 int64Min(Int64 a, Int64 b) {
	return a<b ? a : b;
}

/** Unsigned divide 64-bit integer (software).
 */
Uint64 uint64Div(Uint64 a, Uint64 b);

Uint64 uint64Mod(Uint64 a, Uint64 b);

#ifdef NATIVE_64BIT
/** Signed divide 64-bit integer.
 */
static inline Int64 int64Div(Int64 a, Int64 b) {	return a/b;	}
static inline Int64 int64Mod(Int64 a, Int64 b) {	return a%b;	}

#else
Int64 int64Div(Int64 a, Int64 b);
Int64 int64Mod(Int64 a, Int64 b);
#endif

/** Calculates the integral part log2, rounded up.
 * @param x a value >=1, 0 is allowed as an exception.
 * @return the smallest value l such that 2^l >= x. This value is in the range of 0..64 (yes, 64).
 * If x==0, then -64 is returned.
 */
inline static int uint64Log2Ceil(Uint64 x) {
	if (x==0) return -64;
	for (int l=0; l<64; ++l) if (1llu<<l >= x) return l;
	return 64;
}

/** Calculates the integral part log2, rounded down.
 * @param x a value >= 1.
 * @return the biggest value l such that 2^l <= x . This value is in the range of 0..63.
 * If x==0 then -64 is returned (good approximation of 0 for Int64).
 */
inline static int uint64Log2Floor(Uint64 x) {
	for (int l=63; l>=0; --l) if (1<<l <= x) return l;
	return -64;	// approximation of 0
}

/** Shift function allowing positive and negative counts.
 * @param a the value to shift
 * @param p the number of bits to shift, positive or negative.
 * @return the signed shift result.
 */
static inline Int64 int64MulExp2(Int64 a, int p) {
	if (p>=0) return a << p;
	else return a >> -p;
}

/** Shift function allowing positive and negative counts.
 * @param a the value to shift
 * @param p the number of bits to shift, positive or negative.
 * @return the signed shift result.
 */
static inline Uint64 uint64MulExp2(Uint64 a, int p) {
	if (p>=0) return a << p;
	else return a >> -p;
}

/** Rounds correctly at +0.5 even if the values are negative.
 * @param value the value representation to round. The real value will be value/divider.
 * @param divider the divider used to get the real value.
 * @return a rounded representation, that after dividing will yield the correctly rounded result.
 */
static inline Int64 int64RoundForDivide(Int64 value, Int64 divider) {
	return value + (value>=0 ? divider>>1 : -divider>>1);
}

/** Performs linear interpolation of 32-bit values. Because of huge intermediate values, this calculation requires
 * 64-bit math. That's why it's found here. There are no limitations on the values. This function does NOT perform
 * extrapolation outside the range x0 to x1. y1 and y0 are the result extremes.
 * @param x0 x-value of first defining point.
 * @param y0 y-value of first defining point.
 * @param x1 x-value of second defining point.
 * @param y1 y-value of second defining point.
 * @param x the position of the interpolation request.
 * @return the linearly interpolated value between (including) y0 and y1.
 */
Int32 int32LinearInterpolate(Int32 x0, Int32 y0, Int32 x1, Int32 y1, Int32 x);

/** Performs linear interpolation of 32-bit values. Because of huge intermediate values, this calculation requires
 * 64-bit math. That's why it's found here. Rhis function performs
 * extrapolation outside the range x0 to x1. It is the user's responsibility to avoid range overflows. There will not
 * be any overflows in the range x0 to x1.
 * @param x0 x-value of first defining point.
 * @param y0 y-value of first defining point.
 * @param x1 x-value of second defining point.
 * @param y1 y-value of second defining point.
 * @param x the position of the interpolation request.
 * @return the linearly interpolated value.
 */
Int32 int32LinearExtrapolate(Int32 x0, Int32 y0, Int32 x1, Int32 y1, Int32 x);

/** Fixed point multiplication. No handling of overflows.
 * @param e the binary point position (number of fractional bits)
 * @param a_en a number with e fractional bits
 * @param b_en a number with e fractional bits
 * @return a*b truncated to 32 bits with the same number of fractional bits as the operands
 */
inline static Int32 int32Mul_en (int e, Int32 a_en, Int32 b_en) {
	return (Int64)a_en * b_en >> e;
}

/** Fixed point multiplication of 3 values. No handling of overflows.
 * @param e the binary point position (number of fractional bits)
 * @param a_en a number with e fractional bits
 * @param b_en a number with e fractional bits
 * @param c_en a number with e fractional bits
 * @return a*b*c truncated to 32 bits with the same number of fractional bits as the operands
 */
inline static Int32 int32Mul3_en (int e, Int32 a_en, Int32 b_en, Int32 c_en) {
	return int32Mul_en (e,int32Mul_en (e,a_en, b_en), c_en);
}

/** Fixed point multiplication. No handling of overflows.
 * @param e the binary point position (number of fractional bits)
 * @param a_en a number with e fractional bits
 * @param b_en a number with e fractional bits
 * @return a*b truncated to 32 bits with the same number of fractional bits as the operands
 */
inline static Uint32 uint32Mul_en (int e, Uint32 a_en, Uint32 b_en) {
	return (Uint64)a_en * b_en >> e;
}

/** Convenience function: square value (without loss of precision).
 * @param x signed value
 * @return x*x with proper signedness and bit width
 */
inline static Uint64 int32ToUint64Sqr (Int32 x) {
	return (Int64)x*x;
}

/** Convenience function: square value (without loss of precision).
 * @param x signed value
 * @return x*x with proper signedness and bit width
 */
inline static Uint64 uint32ToUint64Sqr (Uint32 x) {
	return (Uint64)x*x;
}

/** Fixed point division. No handling of divide by zero or overflows. Therefore, b_en has to be close enough to '1' for
 * the result to fit in 32 bits.
 * @param e the binary point position (number of fractional bits)
 * @param a_en a number with e fractional bits
 * @param b_en a number with e fractional bits. Too small values (including 0) generate overflow, resulting in truncated
 *   return values.
 * @return a/b truncated to 32 bits with the same number of fractional bits as the operands. 0 if divide-by-zero.
 */
inline static Int32 int32Div_en (int e, Int32 a_en, Int32 b_en) {
	return int64Div ((Int64)a_en << e,b_en);	
}

/** Calculates the (multiplicative) inverse 1/x.
 * @param e the binary point position (number of fractional bits)
 * @param x_en a number with e fractional bits
 * @return 1/x truncated to 32 bits with the same number of fractional bits as the operand. 0 if divide-by-zero.
 */
inline static Int32 int32Inverse_en (int e, Int32 x_en) {
	return int32Div_en (e, 1<<e, x_en);
}

/** Fixed point division. No handling of divide by zero or overflows. Therefore, b_en has to be close enough to '1' for
 * the result to fit in 32 bits.
 * @param e the binary point position (number of fractional bits)
 * @param a_en a number with e fractional bits
 * @param b_en a number with e fractional bits. Too small values (including 0) generate overflow, resulting in truncated
 *   return values.
 * @return a/b truncated to 32 bits with the same number of fractional bits as the operands. 0 if divide-by-zero.
 */
inline static Uint32 uint32Div_en (int e, Uint32 a_en, Uint32 b_en) {
	return int64Div ((Uint64)a_en << e,b_en);	
}

/* Fixed point division. No handling of divide by zero or overflows. Therefore, b_en has to be close enough to '1' for
 * the result to fit in 64 bits. This function uses 64 bit operations, only, at the expense of precision.
 * @param e the binary point position (number of fractional bits)
 * @param a_en a number with e fractional bits
 * @param b_en a number with e fractional bits. Too small values (including 0) generate overflow, resulting in truncated
 *   return values.
 * @return a/b truncated to 64 bits with the same number of fractional bits as the operands. 0 if divide-by-zero.
 */
Uint64 uint64DivFast_en (int e, Uint64 a_en, Uint64 b_en);

/* Fixed point multiplication. No handling of divide by zero or overflows. Therefore, b_en has to be close enough to '1' for
 * the result to fit in 64 bits. This function uses 64 bit operations, only, at the expense of precision. It's not a cheap
 * function, however, because of many decisions about shifting.
 * @param e the binary point position (number of fractional bits)
 * @param a_en a number with e fractional bits
 * @param b_en a number with e fractional bits. Too small values (including 0) generate overflow, resulting in truncated
 *   return values.
 * @return a/b truncated to 64 bits with the same number of fractional bits as the operands. 0 if divide-by-zero.
 */
Uint64 uint64MulFast_en (int e, Uint64 a_en, Uint64 b_en);

////////////////////////////////////////////////////////////////////////////////////////////////////
// square roots and derived

/** Calculates the integral square root approximation. This function uses 64bits in the calculation.
 * Verified 04/2016.
 * @param r radicant
 * @return the biggest value x such that x*x <= r
 */
Uint32 uint64SqrtFloor (Uint64 r);

/** Calculates the integral square root approximation. This function uses 64bits in the calculation.
 * Untested.
 * @param r radicant
 * @return the biggest value x such that x*x <= r
 */
inline static Uint64 uint64SqrtCeil (Uint64 r) {
	Uint64 root = uint64SqrtFloor(r);
	return root*root >= r ? root : root+1;
}


/** Calculates the fixed point square root approximation. This function requires 64bits in the calculation.
 * Verified 04/2016.
 * @param e the exponent (number of bits right of the point).
 * @param r radicant
 * @return the biggest value x such that x*x <= r
 */
Uint32 uint32SqrtFloor_en (int e, Uint32 r);


/** Calculates the fixed point square root approximation. This function requires 64bits in the calculation.
 * Untested.
 * @param e the exponent (number of bits right of the point).
 * @param r radicant
 * @return the smallest value x such that x*x >= r
 */
inline static Uint64 uint32SqrtCeil_en (int e, Uint32 r) {
	Uint32 root = uint32SqrtFloor_en (e,r);
	return uint32Mul_en (e,root,root) >= r ? root : (Uint64)root+1;	// LSB + 1
}

/** Calculates the fractional square root approximation. This function uses 64 bits only (not 128bits) to trade
 * precision for speed. Result is 32 significant bits at most.
 * @param e the exponent (number of bits right of the point).
 * @param r radicant
 * @return the biggest value x such that x*x <= r
 */
Uint64 uint64SqrtFloorFast_en (int e, Uint64 r);

/** Calculates the fractional square root approximation. This function uses 64 bits only (not 128bits) to trade
 * precision for speed. Result is 32 significant bits at most.
 * In the special case of e=64, this function cannot represent the result (E64) in a Uint64 and the result wraps to 0.
 * @param e the exponent (number of bits right of the point).
 * @param r radicant
 * @return the smallest value x such that x*x >= r
 */
inline static Uint64 uint64SqrtCeilFast_en (int e, Uint64 r) {
	Uint64 root = uint64SqrtFloorFast_en (e,r);
	return root*root >= r ? root : (Uint64)root+1;	// LSB + 1
	
}

/** Calculates the length of a 2-dimensional vector.
 * @param a_en first component, independend of binary point position
 * @param b_en seconde component, same binary point position as a_en
 * @return the (non-negative) length of the vector, binary point position is the same as for the input components.
 */
inline static Uint32 int32AbsDim2 (Int32 a_en, Int32 b_en) {
	return uint64SqrtFloor( int32ToUint64Sqr (a_en) + int32ToUint64Sqr (b_en));
}

/** Very special: divide 1.0 (as E32) by integral 32-bit number n with a E32 result of 64 bits.
 * If you wonder: this is needed for the 64-bit precision iterator.
 * RE-CHECK this function for uneven n!
 * @param n a number between 2 and E32-1
 * @return 1.0 / n as E32 fixed point in the range (2^-64 .. 0.5 -2^-64)
 */
inline static Uint64 uint64E64DivN (Uint32 n) {
	if (n&1) return uint64Div (~0llu,n);		// all 1's is a good approximation of 2^64 ;-) RE-CHECK!
	else return uint64Div (1llu<<63, n/2);
}

/** Very special: divide 1.0 (as E32) by integral 32-bit number n with a E32 result of 64 bits. Rounding
 * towards infinity.
 * If you wonder: this is needed for the 64-bit precision iterator.
 * @param n a number between 2 and E32-1
 * @return 1.0 / n as E32 fixed point in the range (2^-64 .. 0.5 -2^-64)
 */
inline static Uint64 uint64E64DivNCeil (Uint32 n) {
	return uint64Div ((1llu<<63)-1+(n/2), n/2);
}

/** When iterating over curves (like Bezier curves as an example) it's neccessary to have increments that amount to one
 * after millions of iterations - which is plainly impossible with precision when using Uint32 increments.
 */
struct Uint32RangeIterator64 {
	Uint64	dt;
	Uint64	t;
};
typedef struct Uint32RangeIterator64 Uint32RangeIterator64;

#if 0
/** Puts the iterator in undefined (not iterable, not readable) state.
 * @param it the iterator
 */
inline static void uint32RangeIterator64Undefine (Uint32RangeIterator64 *it) {
	it->dt = 0;
}

/** Checks, if iterator is undefined (which can be set by ...uint32RangeIterator64Undefine () ) or not. Undefined is simply
 * another state, distinct from any normal iteration or initialization.
 * @param it the iterator
 * @return true if iterator is in undefined state, false otherwise.
 */
inline static bool uint32RangeIterator64IsUndefined (const Uint32RangeIterator64 *it) {
	return it->dt == 0;
}
#endif

/** Check if this iterator is at t=0.
 * @param it the iterator
 * @return true iff not even the tinyest step (which would perhaps still yield a 32-bit value of 0) has been performed yet.
 */
inline static bool uint32RangeIterator64IsZero (const Uint32RangeIterator64 *it) {
	return it->t == 0;
}

/** Resets the iterator's time, possibly to replay the iteration.
 * @param it the iterator
 */
inline static void uint32RangeIterator64Zero (Uint32RangeIterator64 *it) {
	it->t = 0;
}

/** Sets the iterator to zero and remembers the step size.
 * @param it the iterator
 * @param dt_e64 the 64-bit step size, interpreted as the value dt/2^64 < 1.
 */
inline static void uint32RangeIterator64StartStep (Uint32RangeIterator64 * it, Uint64 dt_e64) {
	it->t = 0;
	it->dt = dt_e64;
}

/** Sets the iterator to zero and calculates the step size in a way, that the iterator's last step is
 * approximately one step from '1'.
 * @param it the iterator
 * @param steps the number of steps from 0 to '1'.
 */
inline static void uint32RangeIterator64StartN (Uint32RangeIterator64 * it, Uint32 steps) {
	it->t = 0;
	//it->dt = uint64E64DivN (steps);
	it->dt = uint64Div ((1llu<<63) + (steps>>1) -1, steps>>1);
}

/** Sets the iterator to zero and calculates the step size in a way, that the iterator's last step is
 * only slighly less than '1', i.e. '1' in good approximation.
 * @param it the iterator
 * @param steps the number of steps from 0 to '1'.
 */
inline static void uint32RangeIterator64StartN1 (Uint32RangeIterator64 * it, Uint32 steps) {
	it->t = 0;
	//it->dt = uint64E64DivN (steps);
	it->dt = uint64Div (1llu<<63,steps>>1);
}

/** Returns the current (32-bit) value of the iterator.
 * @param it the iterator
 * @return current value. This value may be the same (!) even after several iterations, if the step size is pretty
 *   small.
 */
inline static Uint32 uint32RangeIterator64Value (const Uint32RangeIterator64 * it) {
	return (Uint32) (it->t>>32);
}

/** Advances by one step. Please note, that the corresponding value (32-bit, truncated) may not change neccessarily.
 * The only valid criterion for iterator termination is the return value of this function.
 * @param it the iterator
 * @return true, if iterator advanced, false if end of range is reached (and value remains unchanged).
 */
inline static bool uint32RangeIterator64Next (Uint32RangeIterator64 * it) {
	Uint64 tNext = it->t + it->dt;
	if (tNext>=it->t) {
		it->t = tNext;
		return true;
	}
	else return false;
}

#endif

