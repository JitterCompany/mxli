/*
  bezier2Utils.h 
  Copyright 2016 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */
#ifndef __bezier2Utils_h
#define __bezier2Utils_h

/** @file
 * @brief Cartesian Bezier curves of order 2 with fixed point calculation only (64bit, 32bit results). This is a specialization
 * and clean up of former bezierUtils.h . See also: bezier2PolarUtils.h .
 */

#include <bezierN.h>
#include <int32x2Fifo.h>
#include <fifo.h>

////////////////////////////////////////////////////////////
// Drawing/debugging; non-modifying functions.

/** Draws a Bezier2 curve using PostScript native utilities.
 * @param fifo the PostScript output device.
 * @param e fixed point position of the vector components
 * @param curve the points defining the polygon. The polygon is not automatically closed if first and last point are
 *   not the same
 * @return true, if fifo was large enough, false if overflown.
 */
bool int32x2CurvePsDrawBezier2_en (Fifo *fifo, int e, const Int32x2Curve *curve);

/** Doubles the segments of a Bezier2 curve, resulting in a defining polygone closer to the curve.
 * @param curve a curve for both source and destination of the operation.
 * @return true in case of success, false otherwise
 */
bool int32x2CurveBezier2DeCasteljau (Int32x2Curve *curve);

/** Calculates the approximate square deviation of a Bezier2 curve and defining polygon points.
 * @param e the position of the fixed point of the vector components.
 * @param x0 first defining point
 * @param x1 second defining point
 * @param x2 third defining point
 * @return the square of the distance between x1 and the curve value at t=0.5
 */
Uint32 int32Bezier2Unflatness2_en (int e, Int32x2 x0, Int32x2 x1, Int32x2 x2);

/** Calculates the approximate deviation of a Bezier2 curve and defining polygon points.
 * This function is slower than int32Bezier2Unflatness2_en because of the square root needed.
 * @param x0 first defining point
 * @param x1 second defining point
 * @param x2 third defining point
 * @return the distance between x1 and the curve value at t=0.5
 */
Uint32 int32Bezier2Unflatness (Int32x2 x0, Int32x2 x1, Int32x2 x2);

/** Applies the De-Casteljau algorithm until a desired flatness of the Bezier2 curve is reached.
 * @param e the position of the fixed point of the vector components.
 * @param unflatness2_en the maximum allowed (squared) unflatness.
 * @param curve an Int32x2Fifo for both source and destination of the operation.
 * @return the maximum unflatness after the flatten step.
 */
Uint32 int32x2CurveBezier2DeCasteljauFlattenStep_en (int e, Uint32 unflatness2_en, Int32x2Curve *curve);

/** Splits those segments into two, that exceed the given segment length. Only one pass is performed.
 * @param e the position of the fixed point of the vector components.
 * @param maxLength_en the maximum distance between starting point and end point of the Bezier2 segment.
 * @param curve the sequence of Bezier2 segments
 * @return the maximum length after calling this function - it may still be more than the limit.
 */
Uint32 int32x2CurveBezier2LimitLengthDeCasteljauStep_en (int e, Int32 maxLength_en, Int32x2Curve *curve);

/** Calculates the approximated tool-radius compensation of a Bezier2 segment and adds the resulting 1 or 2 Bezier2
 * segments (2 or 4 points) to the curve Fifo.
 * @param e fixed point position of the vector components and radius.
 * @param r_en the tool radius, i.e. the distance of the tool center curve to the Bezier2 segment defined by x0..x2.
 * @param x_1 the point defining the tangent at x0 of the previous segment.
 * @param x0 the starting point of the current segment
 * @param x1 the tangents-defining point
 * @param x2 the final point of the current segment
 * @param curve a Fifo ready to take the calculated Bezier2 segments.
 * @return the number of added segments in case of success, values <=0 in case of errors.
 */
int int32x2Bezier2ToolRadiusStep_en (int e, Int32 r_en, Int32x2 x_1, Int32x2 x0, Int32x2 x1, Int32x2 x2, Int32x2Fifo *curve);

/** Performs tool radius compensation for a curve of multiple Bezier2 segments. The tool is assumed to be perfectly
 * circular. A side effect of tool radius compensation is: the resulting curve does not habe any edges any more.
 * The input curve must contain the initial point (not part of a segment), which is for a closed curve the same value
 * as the final point.
 * The output curve uses the same convention thus includes the start point in the curve data.
 * @param e fixed point position of the vector components and the radius
 * @param r_en the tool radius
 * @param x_1 the point defining the direction of approach of the tool to the first segment's starting point.
 * @param curve input of the uncorrected curve, either closed or open; output of the compensated curve.
 * @return number of resulting segments. This can be roughly double as much as in the original curve.
 */
int int32x2CurveBezier2ToolRadius_en (int e, Int32 r_en, Int32x2 x_1, Int32x2Curve *curve);

/** Performs tool radius compensation for a curve of multiple Bezier2 segments. The tool is assumed to be perfectly
 * circular. A side effect of tool radius compensation is: the resulting curve does not habe any edges any more.
 * The input curve must contain the initial point (not part of a segment), which is for a closed curve the same value
 * as the final point.
 * The output curve uses the same convention thus includes the start point in the curve data.
 * @param e fixed point position of the vector components and the radius
 * @param r_en the tool radius
 * @param curve input of the uncorrected curve, output of the compensated curve.
 * @return number of resulting segments. This can be roughly double as much as in the original curve.
 */
int int32x2CurveBezier2ToolRadiusOptimistic_en (int e, Int32 r_en, Int32x2Curve *curve);

/** Closes a curve using two Bezier2 segments. The resulting curve starts and ends in cp and adds smoothly to the input
 * curve.
 * @param e the fixed point position of the cartesian points
 * @param excentricity_en the degree of 'wasteful' movement to reach the input curve start and end points. A value of 1
 *   leads to a Bezier2 mit point at the intersection of the tangents of the input curve. 0 results in an edge at the
 *   transition points (undesirable) and to degenerated Bezier2 curves (straight lines).
 * @param cp start/end point - the connection point of the curve.
 * @param curve input/output.
 * @return true in case of success, false if the Fifo was exhausted or not sufficient points available to define curve.
 */
bool int32x2CurveBezier2ClosePathAt (int e, Int32 excentricity_en, Int32x2 cp, Int32x2Curve *curve);

/** Closes a curve using two Bezier2 segments. The resulting curve starts and ends in cp and adds smoothly to the input
 * curve.
 * @deprecated
 * @param e the fixed point position of the cartesian points
 * @param excentricity_en the degree of 'wasteful' movement to reach the input curve start and end points. A value of 1
 *   leads to a Bezier2 mit point at the intersection of the tangents of the input curve. 0 results in an edge at the
 *   transition points (undesirable) and to degenerated Bezier2 curves (straight lines).
 * @param cp start/end point - the connection point of the curve.
 * @param curve input/output.
 * @return true in case of success, false if the Fifo was exhausted or not sufficient points available to define curve.
bool int32x2FifoBezier2ClosePathAt (int e, Int32 excentricity_en, Int32x2 cp, Int32x2Curve *curve);
 */

#endif

