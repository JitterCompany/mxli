/*
  int32x2x2.h - 32bit 2x2 matrices with fixed point componenets.
  Copyright 2016 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __int32x2x2_h
#define __int32x2x2_h

/** @file
 * @brief 32bit bit fixed point 2-dimensional vector / complex number.
 */

#include <int32x2.h>
#include <fifo.h>

struct Int32x2x2 {
	Int32 xx;	Int32 xy;
	Int32 yx;	Int32 yy;
};
typedef struct Int32x2x2 Int32x2x2;

/** Vector-matrix multiplication. y = A*x. TESTED.
 * A = ( a[0] a[1] )  x = ( x[0] )
 *     ( a[2] a[3] )      ( x[1] )
 */
static inline Int32x2 int32x2x2MulInt32x2_en (int e, Int32x2x2 a, Int32x2 x) {
	const Int32x2 y = {
		(Int64)a.xx*x.x + (Int64)a.xy*x.y >> e,
		(Int64)a.yx*x.x + (Int64)a.yy*x.y >> e
	};
	return y;
}

/** Matrix-matrix multiplication. C = A*B. TESTED.
 * A = ( a.xx a.xy )  B = ( b.xx b.xy )
 *     ( a.yx a.yy )      ( b.yx b.yy )
 */
inline static Int32x2x2 int32x2x2MulInt32x2x2_en (int e, Int32x2x2 a, Int32x2x2 b) {
	const Int32x2x2 c = {
		(Int64)a.xx*b.xx + (Int64)a.xy*b.yx >> e,	(Int64)a.xx*b.xy + (Int64)a.xy*b.yy >> e,
		(Int64)a.yx*b.xx + (Int64)a.yy*b.yx >> e,	(Int64)a.yx*b.xy + (Int64)a.yy*b.yy >> e,
	};
	return c;
}

/** Multiplies 3 matrices. TESTED.
 */
inline static Int32x2x2 int32x2x2MulTripleInt32x2x2_en (int e, Int32x2x2 a, Int32x2x2 b, Int32x2x2 c) {
	return int32x2x2MulInt32x2x2_en (e,int32x2x2MulInt32x2x2_en (e,a,b), c);
}

/** Rotation matrix, angle counter-clockwise. TESTED.
 */
inline static Int32x2x2 int32x2x2Rot_e32 (int e, Uint32 angle_e32) {
	Int32x2 z = int32x2Rot_en (e,angle_e32);	// complex rotation number
	const Int32x2x2 r = { 	z.x, -z.y,
				z.y, z.x	};
	return r;
}

/** Unit matrix.
 */
inline static Int32x2x2 int32x2x2Unit_en (int e) {
	Int32x2x2 r = {		1<<e,	0,
				0,	1<<e	};
	return r;
}

/** Scales a matrix.
 */
inline static Int32x2x2 int32x2x2Scale_en (int e, Int32x2x2 a, Int32 p_en) {
	const Int32x2x2 r = {		(Int64)a.xx*p_en>>e,	(Int64)a.xy*p_en>>e,
					(Int64)a.yx*p_en>>e,	(Int64)a.yy*p_en>>e	};
	return r;
}

inline static Int32x2x2 int32x2x2Transpose (Int32x2x2 a) {
	const Int32x2x2 r = {	a.xx,	a.yx,
				a.xy,	a.yy	};
	return r;
}

/** Scales a matrix.
 */
inline static Int32x2x2 int32x2x2ScaleXy_en (int e, Int32x2x2 a, Int32 px_en, Int32 py_en) {
	const Int32x2x2 r = {		(Int64)a.xx*px_en>>e,	(Int64)a.xy*px_en>>e,
					(Int64)a.yx*py_en>>e,	(Int64)a.yy*py_en>>e	};
	return r;
}

/** A scaling matrix.
 */
inline static Int32x2x2 int32x2x2ScalingMatrixXy (Int32 px_en, Int32 py_en) {
	const Int32x2x2 r = {		px_en,	0,
					0,	py_en	};
	return r;
}

/** A scaling matrix.
 */
inline static Int32x2x2 int32x2x2ScalingMatrix (Int32 p_en) {
	const Int32x2x2 r = {		p_en,	0,
					0,	p_en	};
	return r;
}

/** Creates a diagonal matrix with components from a vector. Used for anisotropic stretching
 */
inline static Int32x2x2 int32x2x2FromInt32x2 (Int32x2 v) {
	const Int32x2x2 r = {		v.x,	0,
					0,	v.y	};
	return r;
}

/** Creates a Matrix, that performs the same operation as the multiplication with a complex number. This is especially
 * useful for rotations with scaling.
 * @param v the complex number representing the rotation and scaling. Use a unit vector for pure rotations.
 */
inline static Int32x2x2 int32x2x2FromComplex (Int32x2 v) {
	const Int32x2x2 r = {		v.x,	-v.y,
					v.y,	v.x	};
	return r;
}

/** Creates a Matrix, that performs the same operation as the multiplication with a complex number. This is especially
 * useful for rotations with scaling. The direction of rotation is reversed.
 * @param v the complex number representing the inverted rotation and scaling. Use a unit vector for pure rotations.
 */
inline static Int32x2x2 int32x2x2FromComplexConj (Int32x2 v) {
	const Int32x2x2 r = {		v.x,	v.y,
					-v.y,	v.x	};
	return r;
}

inline static Int32 int32x2x2Det_en (int e, Int32x2x2 a) {
	return (Int64)a.xx*a.yy - (Int64)a.yx*a.xy >> e;
}

/**
 */
Int32x2x2 int32x2x2DirectionScalingMatrix_en (int e, Int32x2 v);

/** Inverts a matrix. Make sure, this is possible.
 */
Int32x2x2 int32x2x2Invert_en (int e, Int32x2x2 a);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Affine transformation: Matrix multiplication + offset

struct Int32x2Affine {
	Int32x2x2	matrix;
	Int32x2		shift;
};
typedef struct Int32x2Affine Int32x2Affine;

/** Affine transformation of a single vector.
 * @param e fixed point position
 * @param affine the transformation
 * @param x the vector to transform
 */
Int32x2 int32x2Affine_en (int e, const Int32x2Affine affine, Int32x2 x);

Int32x2Affine int32x2AffineInvert_en (int e, const Int32x2Affine affine);

#endif
