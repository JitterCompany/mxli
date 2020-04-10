/*
  bezierUtils.h 
  Copyright 2016 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */
#ifndef __bezierUtils_h
#define __bezierUtils_h

/** @file
 * @brief Bezier curves of order 4 with fixed point calculation only (64bit, 32bit results).
 */

#include <bezierN.h>
#include <int32x2Fifo.h>
#include <fifo.h>

/** Draws a polygon approximating the Bezier curve.
 * @param fifo the PostScript output device
 * @param e fixed point position of the vector components
 * @param curve the points defining the polygon. The polygon is not automatically closed if first and last point are
 *   not the same
 * @return true, if fifo was large enough, false if overflown.
 */
bool int32x2CurvePsDrawPolygon_en (Fifo *fifo, int e, const Int32x2Curve *curve);

/** Draws a polygon, optionally closing the shape, if first and last point differ.
 * @param fifo the PostScript output device
 * @param e fixed point position of the vector components
 * @param points the points defining the polygon. The polygon is automatically closed if first and last point are
 *   not the same
 * @return true, if fifo was large enough, false if overflown.
 */
bool int32x2FifoPsDrawPolygon_en (Fifo *fifo, int e, const Int32x2Fifo *points);

/** Draws a vector from the origin to every defining point of the curve.
 * @param fifo the output
 * @param e fixed point position of the vector components.
 * @param origin the starting point of each drawn vector
 * @param curve collection of points. The curve points are NOT consumed.
 * @return true, if successfully written, false if fifo overflown.
 */
bool int32x2CurvePsDrawRays_en (Fifo *fifo, int e, Int32x2 origin, const Int32x2Curve *curve);

/** Draws a vector from the origin to every defining point of the curve.
 * @param fifo the output
 * @param e fixed point position of the vector components.
 * @param origin the starting point of each drawn vector
 * @param curve collection of points. The curve points are NOT consumed.
 * @return true, if successfully written, false if fifo overflown.
 */
bool int32x2FifoPsDrawRays_en (Fifo *fifo, int e, Int32x2 origin, const Int32x2Fifo *curve);

/** Draws a vector from the 'sun' (central circle) to every defining point of the curve.
 * @param fifo the output
 * @param e fixed point position of the vector components.
 * @param origin the starting point of each drawn vector
 * @param rSun the radius of 'the sun' emitting the rays.
 * @param curve collection of points. The curve points are NOT consumed.
 * @return true, if successfully written, false if fifo overflown.
 */
bool int32x2FifoPsDrawSunRays_en (Fifo *fifo, int e, Int32x2 origin, Int32 rSun, const Int32x2Fifo *curve);

////////////////////////////////////////////////////////////
// Manipulation

/** Calculates the average of all curve points. If start and end are the same, then it is counted only
 * once.
 */
Int32x2 int32x2CurveAvg (const Int32x2Curve *curve);

/** Calculates a weighted average of all curve points. If start and end are the same, then it is counted only
 * once. It's an approximation of the center of gravity.
 */
Int32x2 int32x2CurveCog (const Int32x2Curve *curve);

/** Calculates the center of gravity of the polygon. Start and end must be the same
 * It's an approximation of the center of gravity of the bezier curve area.
 */
Int32x2 int32x2PolygonAreaCog (const Int32x2Curve *curve);

/** Calculates the miter vector.
 * @param e fixed point position
 * @param a fixed point vector, MUST be of the same length as b
 * @param b fixed point vector, MUST be of the same length as a
 * @return the intersection point
 */
Int32x2 int32x2Miter_en (int e, Int32x2 a, Int32x2 b);

/** Tool offset from a straight line from x0 to x1.
 * @param e fixed point position
 * @param r_en the tool radius of a perfectly circular tool. Positive values result in an offset to the left of the path
 *   from x0 to x1, negative values yield an offset to the right.
 * @param x0 the touching point of the tool. More generally the starting point of the path
 * @param x1 the (Bezier) point defining the future direction of the tool away from x0. More generally the end point of
 *   the path. Must be far enough from x0.
 * @return the displacement of the tool, so that it only touches the line. 
 */
Int32x2 int32x2ToolOffset_en (int e, Int32 r_en, Int32x2 x0, Int32x2 x1);

/** Doubles the segments of a Bezier3 curve, resulting in a defining polygon closer to the curve.
 * @param curve a curve for both source and destination of the operation.
 * @return true in case of success, false otherwise
 */
bool int32x2CurveBezier3DeCasteljau (Int32x2Curve *curve);

/** Doubles the segments of a Bezier2 curve, resulting in a defining polygone closer to the curve.
 * @param curve a curve for both source and destination of the operation.
 * @return true in case of success, false otherwise
 */
bool int32x2CurveBezier4DeCasteljau (Int32x2Curve *curve);


/** Draws a Bezier3 curve using PostScript native primitives.
 * @param fifo the PostScript output device
 * @param e fixed point position of the vector components and the radius
 * @param curve a start point followed by a the sequence of Bezier2 segments
 * @return true, if fifo was large enough, false if overflown.
 */
bool int32x2CurvePsDrawBezier3_en (Fifo *fifo, int e, Int32x2Curve *curve);

/** Draws a polygon-approzimating a curve build up from multiple Bezier4 segments
 * @param fifo the PostScript output device
 * @param e fixed point position of the vector components and the radius
 * @param curve a start point followed by a the sequence of Bezier4 segments
 * @return true, if fifo was large enough, false if overflown.
 */
bool int32x2FifoPsDrawBezier4_en (Fifo *fifo, int e, Int32x2Fifo *curve);

/** Converts polar coordinates into cartesion ones.
 * @param e the fixed point position of the radius component and the resulting cartesian components.
 * @param eAngle the fixed point position of the Angle component.
 * @param p polar coordinates
 * @return the same point in cartesian coordinates
 */
Int32x2 int32x2PolarToCartesian (int e, int eAngle, Int32x2 p);


#endif

