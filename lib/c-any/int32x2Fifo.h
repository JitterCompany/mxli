/*
  int32x2Fifo.h 
  Copyright 2016 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */
#ifndef __int32x2Fifo_h
#define __int32x2Fifo_h

/** @file
 * @brief A specialized Fifo for Int32x2 points, which allows array read/write access of subsequences.
 */

#include <int32x2.h>
#include <int32x2x2.h>	// for affine transform

struct Int32x2Fifo {
	Int32x2		*xs;
	Int32		size;
	Int32		rPos;
	Int32		rTotal;
	Int32		wPos;
	Int32		wTotal;
	Int32		wFragment;	///< fragments discarded by write, because a wraparound would have happened.
	Int32		rFragment;	///< fragments freed by read.
};

typedef struct Int32x2Fifo Int32x2Fifo;

/** Completely empties a Fifo. NOT thread safe!
 */
inline static void int32x2FifoClear (Int32x2Fifo *fifo) {
	fifo->rPos = fifo->rTotal = fifo->wPos = fifo->wTotal = fifo->wFragment = fifo->rFragment = 0;
}

/** Calculates the average of all points - which ist the center of gravity, if all points have the same weight.
 * @param points list of points - unmodified
 * @return the average of all points.
 */
Int32x2 int32x2FifoAvg (const Int32x2Fifo *points);

/** Checks how many elements are free if they can be write one at a time.
 * @param ci the curve iterator.
 */
int int32x2FifoCanRead (const Int32x2Fifo *ci);

/** Checks how many elements are free at the moment if they can be write one at a time.
 * @param ci the curve iterator.
 * @return the number of unused buffer elements.
 */
int int32x2FifoCanWrite (const Int32x2Fifo *ci);

/** Reads and frees one element.
 * @param ci the curve iterator.
 * @return the element just removed.
 */
Int32x2 int32x2FifoRead (Int32x2Fifo *ci);

/** Returns one element without removing it from the Fifo.
 * @param ci the curve iterator.
 * @param n the lookahead distance from the current readable object. Negative values count from the end of the fifo.
 * @return the element NOT removed.
 */
Int32x2 int32x2FifoLookAhead (const Int32x2Fifo *ci, int n);

/** Writes one element. You have to check first, if there's space for it.
 * @param ci the curve iterator.
 * @param e the element to add.
 */
void int32x2FifoWrite (Int32x2Fifo *ci, Int32x2 e);

/** Returns the number of Elements readable as an array.
 * @param ci the curve iterator.
 */
int int32x2FifoCanReadArray (const Int32x2Fifo *ci);

/** Returns the biggest number of Elements, that can be written without a wrap-around, so that these elements may be
 * read as an array later.
 * @param ci the curve iterator.
 */
int int32x2FifoCanWriteArray (const Int32x2Fifo *ci);

/** Provides an array look onto the current readable elements, the number of which you had better checked at this
 * point.
 * @param ci the curve iterator.
 */
const Int32x2* int32x2FifoReadArrayBegin (const Int32x2Fifo *ci);

/** Skips a number of readable elements.
 * @param ci the curve iterator.
 * @param n the number of elements, not neccessarily array-readable.
 */
void int32x2FifoReadArrayEnd (Int32x2Fifo *ci, int n);

/** Provides an array look onto the next length elements as an array. This function can cause unused fragments at the
 * end of the Fifo buffer (until the next read wrap-around).
 * @param ci the curve iterator.
 * @param length the number of elements to write as a (random-access) array.
 * @return a pointer to the writeable array.
 */
Int32x2* int32x2FifoWriteArrayBegin (Int32x2Fifo *ci, int length);

/** Commits the write of length elements and makes them available for reading.
 * @param ci the curve iterator.
 * @param length the number of elements to commit. Must be equal or less than the number used at the start.
 */
void int32x2FifoWriteArrayEnd (Int32x2Fifo *ci, int length);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Curves

/** A curve consisting of a start point and a sequence of points defining the shape.
 */
struct Int32x2Curve {
	Int32x2		x0;		///< start point.
	Int32x2Fifo	*segments;	///< shape defining points; in case of Bezier2: 2*x points continuing the shape from previous positions
};

/** A curve consisting of a start point and a sequence of points defining the shape.
 */
typedef struct Int32x2Curve Int32x2Curve;

/** Returns the curve's final point.
 */
inline static Int32x2 int32x2CurveEnd (const Int32x2Curve *curve) {
	return int32x2FifoCanRead (curve->segments)!=0 ? int32x2FifoLookAhead (curve->segments,-1) : curve->x0;
}

/** Calculates a Curve from the shape-defining points (only) under the assumption, that the curve has to be closed.
 * Therefore the last point is copied and used as start point, too.
 * @param segments points following the start point. Typically pairs, triples, tuples.
 */
inline static Int32x2Curve int32x2FifoClosedCurve (Int32x2Fifo *segments) {
	Int32x2Curve c = {
		int32x2FifoLookAhead (segments,-1),
		segments
	};
	return c;
}

/** Copies a Fifo's contents.
 * @param dst the image. Must provide enough space for successful completion.
 * @param src the source
 * @return true, if dst has enough capacity, false otherwise and dst not modified.
 */
inline static bool int32x2FifoCopy (Int32x2Fifo *dst, const Int32x2Fifo *src) {
	const int n = int32x2FifoCanRead (src);
	if (n*sizeof(Int32x2) <= dst->size) {	// sufficient space after erasure
		int32x2FifoClear (dst);
		for (int i=0; i<n; i++) int32x2FifoWrite (dst, int32x2FifoLookAhead (src,i));
		return true;
	}
	else return false;
}

/** Copies a Curve's contents.
 * @param dst the image. Must provide enough space for successful completion.
 * @param src the source
 * @return true, if dst has enough capacity, false otherwise and dst not modified.
 */
inline static bool int32x2CurveCopy (Int32x2Curve *dst, const Int32x2Curve *src) {
	if (int32x2FifoCopy (dst->segments, src->segments)) {
		dst->x0 = src->x0;
		return true;
	}
	else return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// higher order functions

/** Applies a function to every element of the Fifo.
 */
void int32x2FifoMap (Int32x2Fifo *fifo, Int32x2 (*f)(Int32x2));

/** Applies a function to every element of the Fifo and the start point.
 */
void int32x2CurveMap (Int32x2Curve *curve, Int32x2 (*f)(Int32x2));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Affine transformations.

/** Applies an affine transform to every element of the Fifo.
 */
void int32x2FifoMapAffine_en (int e, Int32x2Fifo *fifo, const Int32x2Affine affine);

/** Applies an affine transform to every element of the Fifo and the start point.
 */
void int32x2CurveMapAffine_en (int e, Int32x2Curve *curve, const Int32x2Affine affine);

/** Convenience function using a matrix only instead an affine transformation.
 */
void int32x2CurveMapMatrix_en (int e, Int32x2Curve *curve, const Int32x2x2 matrix);

/** Convenience function using a matrix only instead an affine transformation.
 */
void int32x2FifoMapMatrix_en (int e, Int32x2Fifo *curve, const Int32x2x2 matrix);

/** Convenience function using a translation only instead an affine transformation.
 */
void int32x2CurveMapTranslate (Int32x2Curve *curve, const Int32x2 trans);

/** Convenience function using a matrix only instead an affine transformation.
 */
void int32x2FifoMapTranslate (Int32x2Fifo *curve, const Int32x2 trans);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fifo element order modifying...

//void int32x2FifoDump (const char *s, Int32x2Fifo const *ci);

/** Rotate the contents of the Fifo. WARNING: this function is NOT thread safe, as it accesses both read side and write
 * side of the Fifo.
 * @param fifo the FIFO list of points
 * @param n the number of positions to advance into reading direction. Negative values rotate into the opposite
 *   direction.
 */
void int32x2FifoRotate (Int32x2Fifo *fifo, int n);

/** Rotates the points of a curve. The curve most probably should be a closed one.
 * WARNING: this function is NOT thread safe, as it accesses both read side and write
 * side of the Fifo.
 * @param curve the points defining the curve.
 * @param n the number of positions to advance into reading direction. Negative values rotate into the opposite
 *   direction.
 */
void int32x2CurveRotate (Int32x2Curve *curve, int n);

/** Reverses the order of the contents of the Fifo. WARNING: this function is NOT thread safe, as it accesses both read side and write
 * side of the Fifo.
 * @param fifo the FIFO list of points
 */
void int32x2FifoReverse (Int32x2Fifo *fifo);

/** Reverses the order of the points defining a curve.
 * @param fifo the FIFO list of points
 */
void int32x2CurveReverse (Int32x2Curve *fifo);

#endif

