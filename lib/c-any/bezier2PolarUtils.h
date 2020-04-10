/*
  bezier2PolarUtils.h 
  Copyright 2016 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */
#ifndef __bezier2PolarUtils_h
#define __bezier2PolarUtils_h

/** @file
 * @brief Polar Bezier curves of order 2 with fixed point calculation only (64bit, 32bit results). This is a specialization
 * and clean up of former bezierUtils.h . See also: bezier2Utils.h .
 */

#include <bezierN.h>
#include <int32x2Fifo.h>
#include <fifo.h>

/** Converts polar coordinates into cartesion ones.
 * @param e the fixed point position of the radius component and the resulting cartesian components.
 * @param eAngle the fixed point position of the Angle component.
 * @param p polar coordinates
 * @return the same point in cartesian coordinates
 */
Int32x2 int32x2PolarToCartesian (int e, int eAngle, Int32x2 p);

/** Calculates the smallest value of the radius of a polar Bezier2 curve. The angle components are not used.
 * Use this function, if you want to detect negative radii.
 * @param p0 the starting point
 * @param p1 the control point defining the tangents
 * @param p2 the end point
 * @return the smallest value of the r component.
 */
bool int32x2PolarBezier2RadiusMin (Int32x2 p0, Int32x2 p1, Int32x2 p2);

/** Draws a polygon approximating the polar defined Bezier curve by using DeCasteljau in polar domain and then
 * converting the polar points into cartesian ones.
 * @param fifo the PostScript output device
 * @param e fixed point position of the vector components and the radius
 * @param eAngle the fixed point position for angular movement; 1.0 corresponds to one revolution
 * @param center the center of rotation
 * @param p0 starting point
 * @param p1 tangents defining point (in polar) cordinates, not cartesian ones!
 * @param p2 final point
 * @return true, if fifo was large enough, false if overflown.
 */
bool int32x2DrawPolarBezier2Segment_en (Fifo *fifo, int e, int eAngle, Int32x2 center, Int32x2 p0, Int32x2 p1, Int32x2 p2);

/** Draws a polygon approximating the polar defined Bezier curve by using DeCasteljau in polar domain and then
 * converting the polar points into cartesian ones.
 * @param fifo the PostScript output device
 * @param e fixed point position of the vector components and the radius
 * @param eAngle the fixed point position for angular movement; 1.0 corresponds to one revolution
 * @param center the center of rotation
 * @param curve a sequence of polar Bezier2 segments
 * @return true, if fifo was large enough, false if overflown.
 */
bool int32x2FifoDrawPolarBezier2Curve_en (Fifo *fifo, int e, int eAngle, Int32x2 center, const Int32x2Curve *curve);


/** Polar conversion of a Bezier2 segment. The curve has to be smooth (like a tool-radius compensated curve, for example).
 * The curve is assumed to keep away from the polar origin, so negative radii should be avoided.
 * @param e fixed point position of the vector components. Must be less or equal to 29.
 * @param eAngle the fixed point position for angular movement; 1.0 corresponds to one revolution
 * @param center the center of rotation
 * @param x0Angle_en the accumulated angle of x0 of this segment (which can be multiple revolutions already).
 * @param xs the starting point, the control point defining the tangents and the end point of the cartesian input curve.
 * @param ps the starting point, the control point defining the tangents and the end point of the polar result curve.
 * @return true in case of success. False if a negative radius is calculated or not polar intersection was obtainable.
 */
bool int32x2PolarStep_en (int e, int eAngle, Int32x2 center, const Int32 x0Angle_en, const Int32x2 xs[3], Int32x2 ps[3]);

/** Translates a complete cartesian curve into polar coords.
 * @param e the fixed point position of the cartesian points and the resulting polar radius. 
 * @param eAngle the fixed point position of the polar angles
 * @param center the rotation point
 * @param x0Angle_en the accumulated angle of x0 of this segment (which can be multiple revolutions already).
 * @param curvePolar polar output curve
 * @param curveCartesian cartesian input curve
 * @return true in case of full success, false otherwise.
 */
bool int32x2CurvePolar_en (int e, int eAngle, Int32x2 center, Int32 x0Angle_en, Int32x2Curve *curvePolar, const Int32x2Curve *curveCartesian);

#endif

