/*
  int32x2.h - 32bit fixed point complex numbers.
  Copyright 2016 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __int32x2_h
#define __int32x2_h

/** @file
 * @brief 32bit bit fixed point 2-dimensional vector / complex number.
 */

#include <stdbool.h>
#include <integers.h>
#include <int64Math.h>
#include <fifo.h>

struct Int32x2 {
	Int32	x;
	Int32	y;
};

typedef struct Int32x2 Int32x2;

/** Constructor.
 * @param x first component
 * @param y second component
 * @return a point value.
 */
inline static Int32x2 int32x2 (Int32 x, Int32 y) {
	Int32x2 v = { x,y };
	return v;
}

/** Although a valid one, this value causes problems in many operations thus is the best candidate for 'undefined'.
 */
inline static Int32x2 int32x2Undefined (void) {
	return int32x2 (1<<31,1<<31);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Integral and functions that don't care about a fixed point

/** Checks, if the vector is defined. This function is more strict than comparison with int32x2Undefined!
 * @param v the vector to test.
 * @return true if both components have values different from int32Undefined.
 */
static inline bool int32x2IsDefined (const Int32x2 v) {
	return v.x!=(1<<31) && v.y!=(1<<31);
}

/** Exact eqaulity.
 */
static inline bool int32x2Eq (Int32x2 a, Int32x2 b) {
	return a.x==b.x && a.y==b.y;
}

static inline Int32x2 int32x2Add (Int32x2 a, Int32x2 b) {
	const Int32x2 v = { a.x+b.x, a.y+b.y };
	return v;
}

static inline Int32x2 int32x2Add3 (Int32x2 a, Int32x2 b, Int32x2 c) {
	return int32x2Add (a, int32x2Add (b,c));
}

static inline Int32x2 int32x2Sub (Int32x2 a, Int32x2 b) {
	const Int32x2 v = { a.x-b.x, a.y-b.y };
	return v;
}

static inline Int32x2 int32x2MulComplex (Int32x2 a, Int32x2 b) {
	const Int32x2 v = { a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x };
	return v;
}

/** 2-dim vector product (which is a scalar value). Contains the sine between the vectors.
 */
static inline Int64 int32x2MulVector (Int32x2 a, Int32x2 b) {
	return (Int64)a.x*b.y - (Int64)a.y*b.x;
}

static inline Int32x2 int32x2Scale (Int32x2 a, Int32 p) {
	const Int32x2 v = { a.x*p, a.y*p };
	return v;
}

static inline Int32x2 int32x2Conj (Int32x2 a) {
	const Int32x2 v = { a.x, -a.y };
	return v;
}

static inline Int32x2 int32x2Conjj (Int32x2 a) {
	const Int32x2 v = { -a.x, a.y };
	return v;
}

static inline Int32x2 int32x2Avg (Int32x2 a, Int32x2 b) {
	const Int32x2 v = { (a.x+b.x)/2, (a.y+b.y)/2 };
	return v;
}

/** Calculates j*v, or v rotated by +90 degrees (left) counter-clockwise.
 */
static inline Int32x2 int32x2j (Int32x2 a) {
	const Int32x2 v = { -a.y, a.x };
	return v;
}
	
/** Calculates -1*v, or v.
 */
static inline Int32x2 int32x2jj (Int32x2 a) {
	const Int32x2 v = { -a.x, -a.y };
	return v;
}
	
/** Calculates j*v, or v rotated by +90 degrees (right) clockwise.
 */
static inline Int32x2 int32x2jjj (Int32x2 a) {
	const Int32x2 v = { a.y, -a.x };
	return v;
}
	
/** Calculates the length. Works for integers AND fixed point values.
 * @param a a vector with fixed point components.
 * @return the square, truncated to 32 bits. No overflow checking.
 */
static inline Int32 int32x2Abs (Int32x2 a) {
	return uint64SqrtFloor ((Int64)a.x * a.x + (Int64)a.y * a.y);
}

static inline Uint32 int32x2Dist (Int32x2 a, Int32x2 b) {
	return int32x2Abs (int32x2Sub (a,b));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Fixed point
//

/** Complex number multiplication.
 * VERIFIED: 05/2016.
 */
static inline Int32x2 int32x2MulComplex_en (int n, Int32x2 a, Int32x2 b) {
	const Int32x2 v = {
		(Int64)a.x*b.x - (Int64)a.y*b.y >> n,
		(Int64)a.x*b.y + (Int64)a.y*b.x >> n
	};
	return v;
}

/** Scalar product. Contains the cosine of the angle between the vectors.
 */
static inline Int32 int32x2MulScalar_en (int n, Int32x2 a, Int32x2 b) {
	return (Int64)a.x*b.x + (Int64)a.y*b.y >> n;
}

/** Scalar product. Contains the cosine of the angle between the vectors.
 */
static inline Int64 int32x2MulScalar_e2n (Int32x2 a, Int32x2 b) {
	return (Int64)a.x*b.x + (Int64)a.y*b.y;
}

/** 2-dim vector product (which is a scalar value). Contains the sine between the vectors.
 */
static inline Int32 int32x2MulVector_en (int n, Int32x2 a, Int32x2 b) {
	return (Int64)a.x*b.y - (Int64)a.y*b.x >> n;
}

/** Scales a vector.
 * VERIFIED: 05/2016.
 * @param n position of the dot = binary digits right to the point.
 * @param a the vector
 * @param p_en a fixed point number, with '1' beeing represented by 1<<n .
 * @return the result truncated to 32 bits.
 */
static inline Int32x2 int32x2Scale_en (int n, Int32x2 a, Int32 p_en) {
	const Int32x2 v = { (Int64)a.x*p_en>>n, (Int64)a.y*p_en>>n };
	return v;
}

/** Scales a vector by 1/p.
 * @param n position of the dot = binary digits right to the point.
 * @param a the vector
 * @param p_en a fixed point number, with '1' beeing represented by 1<<n .
 * @return the result truncated to 32 bits.
 */
static inline Int32x2 int32x2ScaleInverse_en (int n, Int32x2 a, Int32 p_en) {
	const Int32x2 v = { int64Div ((Int64)a.x<<n,p_en), int64Div ((Int64)a.y<<n,p_en) };
	return v;
}

/** Scales a vector.
 * @param n position of the dot = binary digits right to the point.
 * @param a the vector
 * @param p a vector containing the scale factors for x and y .
 * @return the result truncated to 32 bits.
 */
static inline Int32x2 int32x2ScaleXy_en (int n, Int32x2 a, Int32x2 p) {
	const Int32x2 v = { (Int64)a.x*p.x>>n, (Int64)a.y*p.y>>n };
	return v;
}

/** Scales a vector by 1/p.
 * @param n position of the dot = binary digits right to the point.
 * @param a the vector
 * @param p a vector containing the scale factors for x and y .
 * @return the result truncated to 32 bits.
 */
static inline Int32x2 int32x2ScaleXyInverse_en (int n, Int32x2 a, Int32x2 p) {
	const Int32x2 v = { int64Div ((Int64)a.x<<n,p.x), int64Div ((Int64)a.y<<n,p.y) };
	return v;
}

/** Calculates the square of the length.
 * @param n position of the dot = binary digits right to the point.
 * @param a a vector with fixed point components.
 * @return the square, truncated to 32 bits. No overflow checking.
 */
static inline Int32 int32x2Abs2_en (int n, Int32x2 a) {
	return (Int64)a.x * a.x + (Int64)a.y * a.y >> n;
}

/** Approximates a vector of length 1 with the (nearly) same direction.
 * @param n position of the dot = binary digits right to the point.
 * @param a a vector with fixed point components. The length must be in the
 *  range >0 and <2. Convergence is good for small deviations from length 1, only.
 * @result a vector of nearly same direction and length closer to 1.0 .
 */
static inline Int32x2 int32x2NormStep_en (int n, Int32x2 a) {
	const Int32 scale = ((3<<n)-int32x2Abs2_en (n,a)) / 2;
	return int32x2Scale_en (n,a,scale); 
}

/** Complex Multiplication of two (approximately) unit-length vectors to perform a rotation.
 * @param n position of the dot = binary digits right to the point.
 * @param a a unit-length vector with fixed point components. 
 * @param b a unit-length vector with fixed point components. 
 * @return the complex product of a and b with a approximation towards length 1.0 .
 */
static inline Int32x2 int32x2Swirl_en (int n, Int32x2 a, Int32x2 b) {
	return int32x2NormStep_en (n, int32x2MulComplex_en (n,a,b));
}

/** Scales the vector to length 1. 
 * This function requires a 64bit division and a 64bit square root.
 */
static inline Int32x2 int32x2ScaleTo1_en (int n, Int32x2 a) {
	const Int32 abs_en = int32x2Abs (a);
	const Int32x2 v = {
		int64Div ((Int64)a.x<<n,abs_en),
		int64Div ((Int64)a.y<<n,abs_en)
	};
	return v;
}

/** Scales a vector to the given length.
 * @param n position of the dot = binary digits right to the point.
 * @param a the vector
 * @param p_en a fixed point number, with '1' beeing represented by 1<<n .
 * @return the result truncated to 32 bits.
 */
static inline Int32x2 int32x2ScaleTo_en (int n, Int32x2 a, Int32 p_en) {
	return int32x2Scale_en (n, int32x2ScaleTo1_en (n, a), p_en);
}

/** Bidirectional components shift.
 * @param a the vector
 * @param shl the (left) shift count. Negative values cause right shift.
 * @return the vector scaled by 2^shl
 */
Int32x2 int32x2MulExp2 (Int32x2 a, int shl);


static inline bool int32x2IsScalarBetween (Int32x2 a, Int32x2 b, Int32x2 v) {
	const Int32x2 dir = int32x2Sub (b,a);
	return	int32x2MulScalar_e2n (dir,a) <= int32x2MulScalar_e2n (dir,v)
	&&	int32x2MulScalar_e2n (dir,v) <= int32x2MulScalar_e2n (dir,b);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CORDIC - requires 64bit math for sqrt,abs,...

/** Calculates a complex number with half the angle and a stretch between 0 and 2.
 * Formula z+|z| (upper half) and -z-|z| (lower half).
 * @param c a complex number with real part fraction preferably >= -0.5. If precision doesn't matter that much, then
 * it's sufficient to keep away from -1.
 * @return a number with half the complex angle. At Re c < -0.5*abs(c) the result abs becomes smaller than the
 *   original value.
 */
Int32x2 int32x2ComplexHalfAngleQ14 (Int32x2 c);

/** Calculates a complex number with half the angle and a stretch between 0 and 2.
 * @param c a complex number with real part fraction preferably <= 0.5. If precision doesn't matter that much, then
 * it's sufficient to keep away from +1.
 * @return a number with half the complex angle. At Re c >= 0.5*abs(c) the result abs becomes smaller than the
 *   original value.
 * -jz + j|z|
 */
Int32x2 int32x2ComplexHalfAngleQ23 (Int32x2 c);

/** Calculates a complex number with half the angle and a stretch between sqrt(2) and 2.
 * @param c a complex number with real part fraction preferably <= 0.5. If precision doesn't matter that much, then
 * it's sufficient to keep away from +1.
 * @return a number with half the complex angle. 
 */
Int32x2 int32x2ComplexHalfAngle (Int32x2 c);

/** Finds out the quadrant
 */
inline static Int32 int32x2ComplexLogj2Bit (Int32x2 c) {
	return	c.y>=0
		? (c.x>0 ? 0 : 1 )
		: (c.x<=0 ? 2 : 3 )
		;
}

/** Compares the complex (strictly positive) angles (0..2*PI) of two numbers.
 * @param a first complex number
 * @param b first complex number
 * @return true, if the angle of the first complex is less or equal than that the second.
 */
bool int32x2AngleLe (Int32x2 a, Int32x2 b);

/** Generates a CORDIC table of fixed-point complex numbers.
 * VERIFIED: 05/2016, poor performance for e=30, e=29 (negative imaginary values).
 * @param e the position of the fixed point. Range 0..30.
 * @param table storage for the table. The result contains -1 at index 0,
 *   j at index 1, exp (j*PI/2^i) for index i.
 */
void int32x2Cordic32TableCalculate_en (int e, Int32x2 *table);

/** Calculates the purely complex exponential exp (2*PI*j*angle_e32/E32).
 * VERIFIED: 05/2016, e=26, e=30
 * @param e the fixed point position of the table values.
 * @param table_en a cordic table with matching fixed point values.
 * @param angle_e32 the angle measured in full revolutions/2^32 (signed or unsigned).
 * @return cos phi + j sin phi, phi = 2*PI*angle_e32
 */
Int32x2 int32x2RotTable_en (int e, const Int32x2 *table_en, Int32 angle_e32);

/** Calculates the purely complex exponential exp (2*PI*j*angle_e32/E32) scaled to 2^e.
 * Verified: 05/2016, e=30 .. e=4.
 * @param e the fixed point position; must be less or equal to 30.
 * @param angle_e32 the angle measured in full revolutions/2^32 (signed or unsigned).
 * @return cos phi + j sin phi, phi = 2*PI*angle_e32
 */
Int32x2 int32x2Rot_en (int e, Int32 angle_e32);

/** Calculates the purely complex ln/(2*PI*j) aka atan2 of Im and Re.
 * @param e the fixed point position.
 * @param table_en a cordic table with matching fixed point values.
 * @param x the argument
 * @return angle_e32 such that exp2Pij (angle_e32) == x
 */
Int32 int32x2AngleTable_e32 (int e, const Int32x2 *table_en, Int32x2 x);

/** Calculates the purely complex ln/(2*PI*j) aka atan2 of Im and Re.
 * Verified: 05/2016, e=30.
 * @param x the argument
 * @return angle_e32 the fraction of 2*PI of x's complex angle.
 */
Int32 int32x2Angle_e32 (Int32x2 x);

/** Calculates cartesian coordinates from polar ones.
 * VERIFIED: 05/2016.
 * @param eP the fixed point position of the polar radius and the resulting cartesian coordinates.
 * @param eAngle the fixed point position of the polar angle; 1.0 corresponds to one revolution.
 * @param p the polar point.
 * @return the cartesian coordinates of the same point, relative to th polar origin.
 */
Int32x2 int32x2PolarToCartesian_een (int eP, int eAngle, Int32x2 p);

/** Calculates cartesian coordinates from polar ones.
 * VERIFIED: 05/2016.
 * @param e the fixed point position of the polar radius and the cartesian coordinates.
 * @param eAngle the fixed point position of the polar angle; 1.0 corresponds to one revolution.
 * @param x the cartesian point, relative to the polar origin.
 * @return the polar coordinates, with the angle in the range -0.5..+0.5
 */
Int32x2 int32x2CartesianToPolar_een (int e, int eAngle, Int32x2 x);

/** Calculates cartesian coordinates from polar ones. Special attention is paid to the resulting angle - it can cross
 * VERIFIED: 05/2016.
 * the x-axis, resulting in values greater than half a turn.
 * @param e the fixed point position of the polar radius and the resulting cartesian coordinates.
 * @param eAngle the fixed point position of the polar angle; 1.0 corresponds to one revolution.
 * @param p the polar starting point.
 * @param x the cartesian point, relative to the polar origin.
 * @return the polar coordinates, with the angle in a range exceeding -0.5..+0.5
 */
Int32x2 int32x2CartesianToPolarFrom_een (int e, int eAngle, Int32x2 p, Int32x2 x);

/** A cordic table in FLASH, ready to use.
 * VERIFIED: 05/2016.
 */
extern const Int32x2 cordicTable_e30[32];

/** Calculates the intersection point of 2 lines.
 * @param e the fixed point position; must be less or equal to 30.
 * @param a point on the first line
 * @param dirA the direction vector of the first line. It may be beneficial to scale this value to length 1.
 * @param b point on the second line
 * @param dirB the direction vector of the second line. It may be beneficial to scale this value to length 1.
 * @param point the result buffer or 0 if the result is not required
 * @return true, if an intersection point is found, false if the directions are parallel.
 */
bool int32x2IntersectLineLine_en (int e, const Int32x2 a, Int32x2 dirA, Int32x2 b, Int32x2 dirB, Int32x2 *point);

/** Calculates the intersection point of a line and a segment (limited/defined by 2 points).
 * @param e the fixed point position; must be less or equal to 30.
 * @param g point on the first line
 * @param dirG the direction vector of the first line
 * @param a first point of the segment
 * @param b second point of the segment
 * @param point the result buffer or 0 if the result is not required
 * @return true, if an intersection point is found, false if the directions are parallel.
 */
bool int32x2IntersectLineSegment_en (int e, const Int32x2 g, Int32x2 dirG, Int32x2 a, Int32x2 b, Int32x2 *point);

/** Calculates the intersection point of 2 lines, which is supposed be roughly somewhere between a and b.
 * This function is mainly intended for determining the middle (control) point of a Bezier2 curve.
 * @param e the fixed point position; must be less or equal to 30.
 * @param sineLimit_en the allowed sine between the direction vectors. If it gets close to 0 the results become
 *   unpredictable hence (a+b)/2 is used as 'intersection point' instead.
 * @param a point on the first line
 * @param dirA the direction vector of the first line
 * @param b point on the second line
 * @param dirB the direction vector of the second line
 * @return the intersection point
 */
Int32x2 int32x2IntersectLineLineBetween_en (int e, Int32 sineLimit_en, const Int32x2 a, Int32x2 dirA, Int32x2 b, Int32x2 dirB);

bool int32x2IsCriticalIntersectLineLine_een (int e, Int32 sineLimit_en, const Int32x2 dirA, const Int32x2 dirB);

/** Checks, if b lies in the open triangle (towards infinity) defined by the origin 0, a and b.
 * @param a defines the bounding line on one side
 * @param x the vector to test.
 * @param b defines the bounding line on the other side
 */
bool int32x2IsAngularBetween (const Int32x2 a, const Int32x2 x, const Int32x2 b);


////////////////////////////////////////////////////////////////////////////////////////////////////
// Printing.

/** Prints a point as fixed point vector as a complex number notation (with a 'j').
 * @param fifo the output destination
 * @param e fixed point position of the components
 * @param fractional the number of digits behind the point plus the point itself, e.g. 3 for 1.23.
 * @param v the value to display
 * @return true, if successfully written, false if fifo exhausted
 */
bool fifoPrintInt32x2Complex_en (Fifo *fifo, int e, int fractional, Int32x2 v);

/** Prints a point as fixed point vector as a complex number notation (with a 'j').
 * @param fifo the output destination
 * @param ex fixed point position of the x-component 
 * @param ey fixed point position of the y-component
 * @param fractional the number of digits behind the point plus the point itself, e.g. 3 for 1.23.
 * @param v the value to display
 * @return true, if successfully written, false if fifo exhausted
 */
bool fifoPrintInt32x2Complex_een (Fifo *fifo, int ex, int ey, int fractional, Int32x2 v);

/** Standard style of printing a point. Currently I'm in the 'complex' mood.
 * @param fifo the output destination
 * @param e fixed point position of the components
 * @param fractional the number of digits behind the point plus the point itself, e.g. 3 for 1.23.
 * @param v the value to display
 * @return true, if successfully written, false if fifo exhausted
 */
inline static bool fifoPrintInt32x2_en (Fifo *fifo, int e, int fractional, Int32x2 v) {
	return fifoPrintInt32x2Complex_en (fifo,e,fractional,v);
}

/** Prints a point as fixed point vector as a complex number notation (with a 'j').
 * @param fifo the output destination
 * @param v the value to display
 * @return true, if successfully written, false if fifo exhausted
 */
bool fifoPrintInt32x2HexComplex (Fifo *fifo, Int32x2 v);

/** Standard style of printing a point. Currently I'm in the 'complex' mood.
 * @param fifo the output destination
 * @param v the value to display
 * @return true, if successfully written, false if fifo exhausted
 */
inline static bool fifoPrintInt32x2Hex (Fifo *fifo, Int32x2 v) {
	return fifoPrintInt32x2HexComplex (fifo,v);
}

// postscript visualization ( vector2.ps project )

/** Prints a vector's value (array of 2 components) in Postscript syntax.
 * @param fifo the output
 * @param e the fixed point position
 * @param value the vector/complex value
 * @return true, if successfully written, false if fifo exhausted
 */
bool fifoPsPrintComplex_en (Fifo *fifo, int e, Int32x2 value);

/** Prints a vector's value (array of 2 components) in Postscript syntax.
 * @param fifo the output
 * @param s the string contents (no escaping required).
 * @return true, if successfully written, false if fifo exhausted
 */
bool fifoPsPrintString (Fifo *fifo, const char *s);

/** Basic functionality of the following drawing functions: 2 complex numbers and optional label + a function name.
 * @param fifo the output
 * @param e the fixed point position
 * @param c1 the vector/complex value
 * @param c2 the vector/complex value
 * @param label an optional string label for the vector or segment
 * @param function the PostScript function to call
 * @return true, if successfully written, false if fifo exhausted
 */
bool fifoPsDrawComplexBasic_en (Fifo *fifo, int e, Int32x2 c1, Int32x2 c2, const char* label, const char *function);

bool fifoPsDrawComplex_en (Fifo *fifo, int e, Int32x2 where, Int32x2 value, const char* label);
bool fifoPsDrawComplexFromTo_en (Fifo *fifo, int e, Int32x2 from, Int32x2 to, const char* label);

bool fifoPsDrawComplexSegment_en (Fifo *fifo, int e, Int32x2 where, Int32x2 value, const char* label);
bool fifoPsDrawComplexSegmentFromTo_en (Fifo *fifo, int e, Int32x2 from, Int32x2 to, const char* label);

bool fifoPsDrawComplexLine_en (Fifo *fifo, int e, Int32x2 point, Int32x2 direction);

bool fifoPsDrawBezier2_en (Fifo *fifo, int e, Int32x2 x0, Int32x2 x1, Int32x2 x2);
bool fifoPsDrawBezier3_en (Fifo *fifo, int e, Int32x2 x0, Int32x2 x1, Int32x2 x2, Int32x2 x3);

bool fifoPsDrawPolygon_en (Fifo *fifo, int e, const Int32x2 *xs, int n);

bool fifoPsDrawComplexCircle_en (Fifo *fifo, int e, const Int32x2 center, Int32 r);

/** Draws a point mark. Handy instrument to indicate errors graphically.
 */
bool fifoPsDrawComplexMark_en (Fifo *fifo, int e, const Int32x2 p, const char *label);

#endif
