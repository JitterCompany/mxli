
/*
  bt.h 
  Copyright 20016 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __bt_h
#define __bt_h

/** @file
 * @brief Marc's Bezier Tool language
 *
 * Structures and functions for defining movements or paths using Bezier2 curves.
 */

#include <bezierN.h>
#include <int32x2.h>		// Vector ops.
#include <int32x2x2.h>		// Matrix ops.
#include <stringTable.h>
#include <bezier2Command.h>	// FC190 'bstep' array structure

typedef struct {
	Int32Bezier1	x,y;
} BtLine;

typedef struct {
	Int32Bezier2	x,y;
} BtBezier2;

typedef struct {
	const char	*text;
} BtLabel;

typedef struct {
	const char*	text;
} BtComment;

typedef struct {
	int		format;
	const char*	text;
} BtInlined;

enum BtType {
	BT_TYPE_UNDEFINED,
	BT_TYPE_LINE,
	BT_TYPE_BEZIER2,
	BT_TYPE_LABEL,
	BT_TYPE_COMMENT,
	BT_TYPE_INLINED,
};

/** A line segment. It has a color/layer/.. a starting speed and a final speed and the curve description.
 */
typedef struct {
	int	type;
	Int32	v0;
	Int32	v1;
	Int32	level;
	union {
		BtLine		line;		///< formally a Bezier1 segment
		BtBezier2	bezier2;	///< Quadratic bezier.
		BtLabel		label;		///< short text message
		BtComment	comment;	///< really only a comment
		BtInlined	inlined;
	};
} BtElement;

inline static BtElement btElementUndefined (void) {
	BtElement e = {};
	return e;
}

/** Constructor function for Bezier2.
 */
inline static BtElement btElementBezier2 (Int32 v0, Int32 v1, Int32 level, Int32x2 x0, Int32x2 x1, Int32x2 x2) {
	BtElement e = {
		.type=BT_TYPE_BEZIER2,
		.v0=v0, .v1=v1, .level=level,
		{
			.bezier2 = {
				.x = { x0.x, x1.x, x2.x },
				.y = { x0.y, x1.y, x2.y }
			},
		}
	};
	return e;
}

Int32x2 btElementBezier2AtSt (BtElement bezier2, Int32 s, Int32 t);

/** Constructor function for line.
 */
inline static BtElement btElementLine (Int32 v0, Int32 v1, Int32 level, Int32x2 x0, Int32x2 x1) {
	BtElement e = {
		.type=BT_TYPE_LINE,
		.v0=v0, .v1=v1, .level=level,
		{
			.line = {
				.x = { x0.x, x1.x },
				.y = { x0.y, x1.y }
			},
		}
	};
	return e;
}

/** Constructor function.
 */
inline static BtElement btElementLabel (Int32 level, const char* label) {
	BtElement e = {
		.type=BT_TYPE_LABEL,
		.v0=0,.v1=0, .level=level,
		{
			.label = { .text = label },
		}
	};
	return e;
}

inline static BtElement btElementInlined (int format, const char *text) {
	BtElement e = {
		.type=BT_TYPE_INLINED,
		.v0=0, .v1=0, .level=0,
		{
			.inlined = { .text = text, .format = format },
		}
	};
	return e;
}

/** Constructor function.
 */
inline static BtElement btElementComment (Int32 level, const char* comment) {
	BtElement e = {
		.type=BT_TYPE_COMMENT,
		.v0=0,.v1=0, .level=level,
		{
			.comment = { .text = comment },
		}
	};
	return e;
}

inline static bool btElementIsDefined (BtElement e) {
	return e.type != BT_TYPE_UNDEFINED;
}

inline static bool btElementIsPath (BtElement e) {
	switch(e.type) {
		case BT_TYPE_LINE:
		case BT_TYPE_BEZIER2:	return true;
		default:		return false;
	}
}

/** If the element is a path element, then the result is the starting point of that path.
 * @param e a path element (line, bezier, ...)
 * @return starting point of path, undefined otherwise
 */
inline static Int32x2 btElementPointBegin (BtElement e) {
	switch(e.type) {
		case BT_TYPE_LINE:	return int32x2 (e.line.x[0], e.line.y[0]);
		case BT_TYPE_BEZIER2:	return int32x2 (e.bezier2.x[0], e.bezier2.y[0]);
		default:		return int32x2Undefined ();
	}
}

/** If the element is a path element, then the result is the final point of that path.
 * @param e a path element (line, bezier, ...)
 * @return final point of path, undefined otherwise
 */
inline static Int32x2 btElementPointEnd (BtElement e) {
	switch(e.type) {
		case BT_TYPE_LINE:	return int32x2 (e.line.x[1], e.line.y[1]);
		case BT_TYPE_BEZIER2:	return int32x2 (e.bezier2.x[2], e.bezier2.y[2]);
		default:		return int32x2Undefined ();
	}
}

typedef struct {
	Int32x2		v0;		///< speed at point x0
	Int32x2		v2;		///< speed at point x2
	Int32x2		a;		///< acceleration
} BtBezier2Dynamics;

/** Calculates (native Bezier) speed and acceleration for a line or Bezier2 elements.
 * @return the kinematic values; values are invalid for other types of elements.
 */
BtBezier2Dynamics btElementBezier2Dynamics (BtElement e);

typedef struct {
	Int32	v;		///< desired speed
	Int32	level;
	Int32x2	x;
	Int32x2	t;		///< tangent; bezier native speed including coefficients
} BtCurveTangent;

BtCurveTangent btElementCurveTangentEnd (BtElement e);
BtCurveTangent btElementCurveTangentBegin (BtElement e);

/** Approximates the path of 2 elements by one element, if possible.
 * @param homogenous true if only the same types (line-line, bezier2-bezier2) may be fused, false if (line-bezier2) is allowed, too.
 * @param errorLimit the tolerable deviations.
 * @param a first element
 * @param b second element
 * @return the element replacing a and b if successful; btElementUndefined() if a and b are not fusible.
 */
BtElement btElementFuse (bool homogenous, Int32 errorLimit, BtElement a, BtElement b);

/** Calculates the element which 'travels' the opposite direction.
 */
BtElement btElementReverse (BtElement e);

struct BtPolarContext {
	Int32x2		center;
	Int32		accumulatedAngle;	///< If set to int32Undefined() then use the initial canonical angle of the first element
	Int32		unitAngle;		///< angles are stored in E20 fixed point; this value provides additional scaling
};
typedef struct BtPolarContext BtPolarContext;

/** Sets the initial angle for a polar conversion.
 * @param context configuration and state of the polar conversion.
 * @param startAngle the angle to be assumed for the first BtElement of the conversion. This can be multiple revolutions forward/backward.
 *   Set to int32Undefined() if the first elements 'natural' angle (up to +-180 degrees) should be used instead.
 */
inline static void btPolarContextReset (BtPolarContext *context, Int32 startAngle) {
	context->accumulatedAngle = startAngle;
}

Int32x2 btElementCanonicalPolarAngles (Int32x2 center, Int32 unitAngle, BtElement e);
Int32 btFullRevolutionsCompensation (Int32 angleScale, Int32 accumulatedAngle, Int32 phase);

/** Cartesian to polar conversion.
 * Polar conversion of a Bezier2 segment. The curve has to be smooth (like a tool-radius compensated curve, for example).
 * The curve is assumed to keep away from the polar origin, so NEVER will negative radii be used.
 * There are 4 different speeds in this concepts: The native cartesian Bezier curve speed, that is small for small extends
 * of the curve and big for large extends of the curve. The same is true for the native polar Bezier curve speed. Then there's
 * the desired cartesian Bezier curves physical speed, that results from scaling up/down the native speed. Finally there's the
 * polar curves desired speed - probably the one most elusive to human comprehension - which is the speed required in polar coordinates
 * to yield the desired physical speed in the cartesian reality. Speed annotations in BT language are desired speeds. They always
 * do a uniform scaling of the native coordinate speeds, by a factor desired/native-speed and nothing more.
 * @param context configuration and state of the polar conversion.
 * @param element the line/bezier2 curve to convert.
 * @return a Bezier2 element in case of successful path element conversion; the same element for a non-path element;
 *   btElementUndefined() otherwise.
 */
BtElement btElementCartesianToPolar (BtPolarContext *context, BtElement element);

/**
 * Polar to cartesian conversion of a Bezier2 segment.
 * The curve is assumed to keep away from the polar origin, so NEVER will negative radii be used.
 * @param context configuration and state of the polar conversion.
 * @param element the line/bezier2 curve to convert.
 * @return a Bezier2 element in case of successful path element conversion; the same element for a non-path element;
 *   btElementUndefined() otherwise.
 */
BtElement btElementPolarToCartesian (BtPolarContext *context, BtElement element);

struct BtEllipticContext {
	Int32x2		centerA;	///< the first center
	Int32x2		centerB;	///< the second center
	bool		below;		///< when converting elliptic -> cartesian, this indicates the half plane
};
typedef struct BtEllipticContext BtEllipticContext;

/** Converts a Bezier element given in cartesian coordinates into elliptic coordinates.
 * NOT FULLY TESTED.
 * @param context configuration and active half-plane
 * @param e the path element
 * @return a path element with the distances to centerA and centerB as cooridinates.
 */
BtElement btElementCartesianToElliptic (BtEllipticContext *context, BtElement e);

/** Converts a Bezier element given in cartesian coordinates into elliptic coordinates.
 * TEST FAILED (2016-12-12).
 * @param context configuration and active half-plane
 * @param e the path element with elliptic coordinates.
 * @return a path element with the distances to centerA and centerB as cooridinates.
 */
BtElement btElementEllipticToCartesian (BtEllipticContext *context, BtElement e);

// A list of BtElements, implemented by BlockFifo.

#include <blockFifo.h>

typedef BlockFifo BtList;

inline static void btListInit (BtList *list, BtElement *elementBuffer, size_t bufferSize) {
	blockFifoInit (list,elementBuffer,bufferSize,sizeof(BtElement));
}

inline static size_t btListCanRead (const BtList *list) {
	return blockFifoCanRead (list);
}

inline static size_t btListCanWrite (const BtList *list) {
	return blockFifoCanWrite (list);
}

inline static void btListWrite (BtList *list, BtElement element) {
	BtElement *pe = (BtElement*) blockFifoWriteLock (list);
	*pe = element;
	blockFifoWriteRelease (list);
}

inline static BtElement btListRead (BtList *list) {
	const BtElement e = * (const BtElement*) blockFifoReadLock (list);
	blockFifoReadRelease (list);
	return e;
}

inline static BtElement btListHead (BtList *list) {
	const BtElement e = * (const BtElement*) blockFifoReadLock (list);	// no side-effect of this operation
	return e;
}

/** Like btListWrite, only the testing is performed by the function before.
 * @param list the destination
 * @param element the element to add
 * @return true, if the element was added, false if the list was already full.
 */
inline static bool btListWriteSafe (BtList *list, BtElement element) {
	if (btListCanWrite (list)) {
		btListWrite (list,element);
		return true;
	}
	else return false;
}

/** Direction reversal of all paths.
 * @param list the list to in-place reverse.
 */
void btListReverse (BtList *list);

/** Appends a point at the end of the list's path.
 * @param list the current curve
 * @param p the new final point of the curve
 * @param excentricity controls the exit speed at the current final point.
 *   A value of 0 causes a straight line. A negative value results in a on-the-point U-turn in the current final
 *   point. Only positive values result in a smooth transition between the segments.
 * @return true in case of successfull add, false otherwise. It is considered an error, if the original path is
 *   empty (hence no point can be added).
 */
bool btListAppendPathPointWithExcentricity (BtList *list, Int32x2 p, Int32 excentricity);

/** Inserts a point before the beginning of the list's path.
 * @param list the current curve
 * @param p the new final point of the curve
 * @param excentricity controls the exit speed at the current final point.
 *   A value of 0 causes a straight line. A negative value results in a on-the-point U-turn in the current final
 *   point. Only positive values result in a smooth transition between the segments.
 * @return true in case of successfull add, false otherwise. It is considered an error, if the original path is
 *   empty (hence no point can be added).
 */
bool btListPrependPathPointWithExcentricity (BtList *list, Int32x2 p, Int32 excentricity);
bool btListAppendPathPointWithDirection (BtList *list, Int32x2 p, Int32x2 direction);
bool btListPrependPathPointWithDirection (BtList *list, Int32x2 p, Int32x2 direction);

/** In-place update of list.
 */
void btListMap (BtList *list, BtElement (*f)(BtElement e));

/** Turns lines into Bezier2 elements simulating lines.
 */
BtElement btLineToBezier2 (BtElement e);

/** 
 */
bool btListExpand (BtList *list, bool (*expand)(BtList *dest, BtElement e));

/** Expands one BtElement into subsegments, until the polygon approximation error drops below the given limit.
 * @param dest the BtElement list used to store the multiple segments result.
 * @param e the element to expand. If it's not a Bezier curve, then it's added unmodified to the result list.
 * @param maxError the maximum absolute difference of any coordinate at point t=0.5 from the (x0+x2)/2 .
 * @return true, if all went fine, false if output list was exhausted - in which case the list's shape is broken.
 */
bool btExpandDeCasteljau (BtList *dest, BtElement e, Uint32 maxError);

/** Expands one BtElement into subsegments, until the segment length is below a given limit.
 * @param dest the BtElement list used to store the multiple segments result.
 * @param e the element to expand. If it's not a Bezier curve, then it's added unmodified to the result list.
 * @param maxLength the maximum length of the segment x0-x1 or x2-x1
 * @return true, if all went fine, false if output list was exhausted - in which case the list's shape is broken.
 */
bool btExpandDeCasteljauLength (BtList *dest, BtElement e, Uint32 maxLength);

/** Expands one BtElement into subsegments, until the segment satisfies the given predicate.
 * @param dest the BtElement list used to store the multiple segments result.
 * @param e the element to expand. If it's not a Bezier curve, then it's added unmodified to the result list.
 * @param predicate a function telling, if an element is 'good enough'.
 * @return true, if all went fine, false if output list was exhausted - in which case the list's shape is broken.
 */
bool btExpandDeCasteljauPredicate (BtList *dest, BtElement e, bool (*predicate)(BtElement e));

/** Expands Bezier elements to lines.
 * @param dest the BtElement list used to store the multiple segments result.
 * @param e the element to expand. If it's not a Bezier curve, then it's added unmodified to the result list.
 * @return true, if all went fine, false if output list was exhausted - in which case the list's shape is broken.
 */
bool btExpandToLines (BtList *dest, BtElement e);

/** Performs a tool radius compensation, i.e. calculates a curve, that is (approx.) r_e20 to the right side of the
 * original curve. The tool is considered to be a full circle.
 * As a side effect, the resulting curve is smooth even for edgy input, if the radius is positive for curve bending
 * to the left or if the radius is negative and the curve bends to the right.
 * @param dest the BtElement list used to store the multiple segments result.yy
 * @param r_e20 the tool radius, i.e. the distance of the compensated curve.
 * @param w1 the previous point on the original curve; used to calculate the direction of the tangent to the
 *   previous segment. This is updated on return. w1 MUST NOT be NULL, if e is a path element.
 *   It can be identical to the first point of the element e, or int32x2Undefined() however.
 * @param e the curve element to compensate.
 * @return true in case of success, false otherwise.
 */
bool btToolRadiusCompensateRight (BtList *dest, Int32 r_e20, Int32x2 *w1,  BtElement e);

////////////////////////////////////////////////////////////////////////////////////////////////////
// intersection with lines / splitting

typedef struct {
	int	segmentIndex;
	Int32x2	point;
} BtHitPoint;

/** HitPoint list object.
 */
typedef BlockFifo BtHitPointFifo;

inline static void btHitPointFifoInit (BtHitPointFifo *fifo, BtHitPoint *buffer, size_t bufferSize) {
	blockFifoInit (fifo, buffer,bufferSize,sizeof(BtHitPoint));
}

inline static bool btHitPointFifoWriteSafe (BtHitPointFifo *fifo, int segmentIndex, Int32x2 point) {
	if (blockFifoCanWrite (fifo)) {
		BtHitPoint *hp = blockFifoWriteLock (fifo);
		hp->segmentIndex = segmentIndex;
		hp->point = point;
		blockFifoWriteRelease (fifo);
		return true;
	}
	else return false;
}

inline static BtHitPoint btHitPointFifoRead (BtHitPointFifo *fifo) {
	const BtHitPoint *hp = blockFifoReadLock (fifo);
	BtHitPoint p = *hp;
	blockFifoReadRelease (fifo);
	return p;
}

inline static bool btHitPointFifoCanFindHitPoint (const BtHitPointFifo *fifo, int segmentIndex) {
	BtHitPointFifo clone = *fifo;
	while (blockFifoCanRead (&clone)) {
		BtHitPoint hp = btHitPointFifoRead (&clone);
		if (hp.segmentIndex==segmentIndex) return true;
	}
	return false;
}

inline static Int32x2 btHitPointFifoFindHitPoint (BtHitPointFifo *fifo, int segmentIndex) {
	Int32x2 point;
	for (int n=blockFifoCanRead (fifo); n>0; n--) {
		BtHitPoint hp = btHitPointFifoRead (fifo);
		if (hp.segmentIndex==segmentIndex) {
			point = hp.point;
		}
		else btHitPointFifoWriteSafe (fifo,hp.segmentIndex,hp.point);	// put it back
	}
	return point;
}

/** Structure for describing a ray and behaviour of interaction with the shape.
 */
struct BtBeam {
	Int32x2		source;
	Int32x2		direction;
	bool		forwardOnly;
	bool		firstHitOnly;
};
typedef struct BtBeam BtBeam;

/** Calculates the intersection point of a line/curve and a 'beam' from point origin.
 * Only the first contact point (viewing) in beam direction is calculated and there can be at most one such point.
 * This function yields points all along the beam's line, even 'behind' the origin, so make sure, the origin is
 * outside the curve for intuitive results.
 * NOT FULLY TESTED.
 * @param e the curve element exposed to the beam
 * @param origin_e20 the source of the beam
 * @param beam_e20 the beam direction. Length doesn't matter here. Negating this parameter yields the points further
 *   away from origin.
 * @return the point where the beam hits the curve, or int32Undefined() if no such point exists.
 */
Int32x2 btElementIntersectBeam (BtElement e, Int32x2 origin_e20, Int32x2 beam_e20);

/** Applies one beam to the curve stored in list.
 * @param beam the beam definition and intersection options.
 * @param list the shape
 * @param hitPoints an empty list to store the hit points.
 * @return true in case of successful calculation, false if some buffer overflow happened.
 */
bool btApplyBeam (BtBeam beam, const BtList *list, BtHitPointFifo *hitPoints);


/** Structure for describing a parallel 'radar wave-front' to detect bounding box, touching points, etc.
 */
struct BtTouch {
	Int32x2		direction;	///< normal vector or tangents - depending on mode
	bool		forwardOnly;	///< direction must match, reverse not allowed
	bool		firstHitOnly;	///< use the closest point (viewing in tangent direction) only
	bool		distanceOnly;	///< edges are not counted; if false, then edges are counted as tangent points
};
typedef struct BtTouch BtTouch;

/** Finds the point on a path element that has a tangent of the desired angle and direction (+-).
 * (NOT FULLY) TESTED 12/2016
 * @param eps the value so small, that we consider it ZERO.
 * @param tangent the Bezier path element local speed orientation and direction (forward, only)
 * @param pathElement a path element
 */
Int32x2 btElementTouchTangent (Int32 eps, Int32x2 tangent, BtElement pathElement);

/** Finds the minimum distance point on the curve, when looking in direction. Edges are frequently returned as such
 * points. This function can be used to calculate bounding boxes.
 * BUG: doesn't work!?
 * @param eps the distance of 2 values considered 'equal'.
 * @param direction measurement normal vector.
 * @param pathElement a path element
 */
Int32x2 btElementTouchDistance (Int32 eps, Int32x2 direction, BtElement pathElement);

/** Applies a touch to the curve stored in list.
 * @param eps the value so small, that we consider it ZERO.
 * @param touch the touch definition and intersection options.
 * @param path the shape
 * @param resultHitPoints an empty list to store the hit points.
 * @return true in case of successful calculation, false if some buffer overflow happened.
 */
bool btApplyTouch (Int32 eps, BtTouch touch, const BtList *path, BtHitPointFifo *resultHitPoints);

struct BtSplitContext {
	StringTable*	stringTable;	///< used for generating new comments and labels
	Int32 		eps;		///< minimum distance of distinguishable points
	bool		doLabel;	///< should split points be labeled?
	bool		doComment;	///< should split points be commented?
	const char*	textLabel;	///< label name
	const char*	textComment;	///< comment text
	int		index;		///< running number of split point
};
typedef struct BtSplitContext BtSplitContext;

/** Splits a line/curve in two at the given point, which is assumed to lie on the line/curve.
 * NOT FULLY TESTED.
 * @param context the BtSplitContext config and state
 * @param dest the BtElement list used to store the multiple segments result.
 * @param p_e20 a point on the curve.
 * @param e the curve element to split.
 * @return true in case of success, false otherwise.
 */
bool btElementSplitAt (BtSplitContext *context, BtList *dest, Int32x2 p_e20, BtElement e);

/** Performs splits an a shape.
 * BUG: currently, only one hit point is allowed per path element.
 * @param splitContext the BtSplitContext config and state
 * @param list the BtElement list to modify
 * @param hitPoints the points (on the shape!) that shall split the curve's segments.
 * @return true in case of success, false otherwise.
 */
bool btListSplitAtHitPoints (BtSplitContext *splitContext, BtList *list, BtHitPointFifo *hitPoints);


////////////////////////////////////////////////////////////////////////////////////////////////////
// Parser/BT language context

enum {
	BT_INVALID_LEVEL = -1,
	BT_DEFAULT_LEVEL = 0,
};

// BezierTools Version 1, main parser.
// This parse transforms an incremental description into a sequence of line segments, each with full information
// about level, speed, starting point, ...

typedef struct {
	const char *	fileName;	///< file name for error reporting
	int		lineNr;		///< line number for error reporting
	Fifo*		errorOutput;	///< channel for writing error messages
	StringTable*	stringTable;	///< for storing comments, symbols, etc
	int		level;
	Int32x2		p0;		///< end of previous path = origin of current path
	Int32		v0,v1;			///< starting speed, final speed of next segment
	Int32x2		scale_e20;		///< x/y scaling, non-accumulating
	Int32x2		translation_e20;	///< x/y translation, non-accumulating
	Int32		rotation_e20;		///< angular rotation, non-accumulating
	Int32x2		initialScale_e20;
	Int32x2		initialTranslation_e20;
	Int32		initialRotation_e20;
	//Int32x2x2	matrix_e20;		///< Combined scaling and rotation.
} BtParserContext;

/** Reports an error.
 * @param ctx state (input side effects) and error output channel, etc.
 * @param msg the error description
 * @param dump the remainder of the parsed line
 */
bool btErrorHere (BtParserContext *ctx, const char *msg, Fifo *dump);

/** Reports an internal error caused by too many Bezier elements.
 * @param ctx state (input side effects) and error output channel, etc.
 * @param dump the remainder of the parsed line
 */
bool btErrorHereListOverflow (BtParserContext *ctx, Fifo *dump);

/** Parses one line. If you add additional commands to the language, then check the line for non-emptyness and parse after return from
 * this function.
 * @param line the input
 * @param list the output list of BtElements
 * @param context state (input side effects) and error output channel, etc.
 * @return true, if line was without error (so far, but maybe not yet processed command), false if an error happened.
 */
bool btParseLine (Fifo *line, BtList *list, BtParserContext *context);


////////////////////////////////////////////////////////////////////////////////////////////////////
// BezierTool output

/** Context for output in BezierTool language.
 */
struct BezierToolContext {
 	Fifo*	output;		///< output the destination buffer
	Int32	level;		///< level of previous actions
	Int32x2	p0;		///< current point or int32x2Undefined()
	Int32	v0;		///< current speed
	bool	hex;		///< hex output instead of E20 decimals
	bool	noComments;	///< suppress comment output
};
typedef struct BezierToolContext BezierToolContext;

/** Resets the context. Writes the file's header, including scale.
 * @param context the accumulated side effects of the previous elements.
 */
bool btBezierToolBegin (BezierToolContext *context);

/** Writes a text representation of the next element.
 * @param context the accumulated side effects of the previous elements.
 * @param element one Bezier segment.
 */
bool btBezierToolElement (BezierToolContext *context, BtElement element);

/** Writes the file's footer.
 * @param context the accumulated side effects of the previous elements.
 */
bool btBezierToolDone (BezierToolContext *context);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Stateless BezierTool output - each line contains full context.
// This is more or less the internale representation of the data.

struct StatelessBezierToolContext {
 	Fifo*	output;		///< output the destination buffer
	bool	hex;		///< hex output instead of E20 decimals
	bool	noComments;	///< suppress comment output
};
typedef struct StatelessBezierToolContext StatelessBezierToolContext;

/** Writes a text representation of the next element.
 * @param context the accumulated side effects of the previous elements.
 * @param element one Bezier segment.
 */
bool btStatelessBezierToolElement (StatelessBezierToolContext *context, BtElement element);


////////////////////////////////////////////////////////////////////////////////////////////////////
// Postscript output

typedef struct {
	Fifo*		output;
	bool		inPath;			///< true, if a path was already started
	Int32		level;			///< level of the previous actions
	Int32x2		p0;			///< current position or ray origin
	bool		literalLabels;		///< should labels be printed literally in Postscript?
	bool		noHeaders;		///< avoid (absolute) header in output - useful for appending code!
	bool		noFooters;		///< avoid (absolute) footer in output - useful for appending code!
	Int32		coordinate;		///< print a coordinate system into the output, unit = value of this param.
	Int32		format;			///< different outputs: shape (normal), rays
	Int32x2		origin;			///< psrays origin (polar origin)
	Int32x2		pageOrigin;		///< placement of the coordinate system origin on the A4 sheet
	Int32x2		zoomOrigin;		///< magnification origin, default 0,0
	Int32		pageZoom;		///< zoom factor (E20)
} BtPostscriptContext;

/** Resets the context. Writes the Postscript file's header: scaling to mm and centor of A4.
 * @param context the accumulated side effects of the previous elements.
 */
bool btPostscriptBegin (BtPostscriptContext *context);

/** Writes a text representation of the next element. Automatic selection of style (format).
 * @param context the accumulated side effects of the previous elements.
 * @param element one Bezier segment.
 */
bool btPostscriptElement (BtPostscriptContext *context, BtElement element);

/** Writes a text representation of the next element.
 * @param context the accumulated side effects of the previous elements.
 * @param element one Bezier segment.
 */
bool btPostscriptElementPath (BtPostscriptContext *context, BtElement element);

/** Writes a text representation of the next element (as rays to the control points).
 * @param context the accumulated side effects of the previous elements.
 * @param element one Bezier segment.
 */
bool btPostscriptElementRay (BtPostscriptContext *context, BtElement element);

/** Writes the file's footer.
 * @param context the accumulated side effects of the previous elements.
 */
bool btPostscriptDone (BtPostscriptContext *context);

////////////////////////////////////////////////////////////////////////////////////////////////////
// G-Code output
//
// currently, all elements are interpolated by straight lines. G64 is assumed for smoothing
// level 0 means G0
// level !=0 means G1
//

typedef struct {
	Fifo*		output;
	const char	axes[2];		///< letters for axes: XY or XZ or ..
	Int32		n;			///< Nxxx line numbering
	bool		inPath;			///< indicates if previous point is valid
	Int32		level;			///< level encoding 0 = positioning (G0), 1=working
	bool		literalLabels;		///< literally include labels into output
	bool		noHeaders;		///< avoid (absolute) header in output - useful for appending code!
} BtGcodeContext;

/** Resets the context. Writes the Gcode file's header: scaling to mm and centor of A4.
 * @param context the accumulated side effects of the previous elements.
 */
bool btGcodeBegin (BtGcodeContext *context);

bool btGcodeN (BtGcodeContext *context, bool print);

/** Writes a text representation of the next element.
 * @param context the accumulated side effects of the previous elements.
 * @param element one Bezier segment.
 */
bool btGcodeElement (BtGcodeContext *context, BtElement element);

/** Writes the file's footer.
 * @param context the accumulated side effects of the previous elements.
 */
bool btGcodeDone (BtGcodeContext *context);

////////////////////////////////////////////////////////////////////////////////////////////////////
// BezierCommand
//
struct BtB2cContext {
	Fifo*		output;
	const char*	name;			///< array name
	bool		enumBeginEnd;		///< include an enum definition of beginning and end points
	bool		hex;			///< hex numbers instead of fixed-point decimals
	bool		noHeaders;		///< avoid (absolute) header in output - useful for appending code!
	Int32x2		curveBegin;		///< very first point of whole file, int32x2Undefined if no path in file
	Int32x2		curveEnd;		///< very last point of whole file, int32x2Undefined if no path in file
};

typedef struct BtB2cContext BtB2cContext;

/** Resets the context. Writes the Gcode file's header: scaling to mm and centor of A4.
 * @param context the accumulated side effects of the previous elements.
 */
bool btB2cBegin (BtB2cContext *context);

/** Writes a text representation of the next element.
 * @param context the accumulated side effects of the previous elements.
 * @param element one Bezier segment.
 */
bool btB2cElement (BtB2cContext *context, BtElement element);

/** Writes the file's footer.
 * @param context the accumulated side effects of the previous elements.
 * @return true in case of successful write, false otherwise
 */
bool btB2cDone (BtB2cContext *context);


////////////////////////////////////////////////////////////////////////////////////////////////////
// generic output function

extern const char* outputFormatSymbols[];

enum OutputFormat {	// index within array a few lines above
	OUTPUT_FORMAT_BT,
	OUTPUT_FORMAT_SBT,
	OUTPUT_FORMAT_PS_RAYS,	///< debugging output: rays to control points
	OUTPUT_FORMAT_PS,	///< normal PS shape output
	OUTPUT_FORMAT_GCODE,
	OUTPUT_FORMAT_B2C,
	OUTPUT_FORMAT_SYMBOLS,	///< key/value pairs of important data like start, end, labels 
};

struct BtOutputContext {
	BezierToolContext		bt;
	StatelessBezierToolContext	sbt;
	BtPostscriptContext		ps;
	BtGcodeContext			gcode;
	BtB2cContext			b2c;
};

typedef struct BtOutputContext BtOutputContext;

bool btOutput (BtOutputContext *context, BtList *btList, int outputFormat);

// better portablility...
struct BtOutputContextSimple {
	Fifo*		output;
	const char*	name;			///< output name
	const char*	axes;			///< XY or XZ in gcode
	bool		optionalOutput;		///< include an enum definition of beginning and end points
	bool		noHeaders;		///< avoid (absolute) header in output - useful for appending code!
	bool		noFooters;		///< avoid (absolute) footer in output - useful for appending code!
	bool		noComments;		///< avoid comments in output
	bool		hex;			///< hex numbers instead of fixed-point decimals
	bool		literalLabels;
	Int32		coordinate;		///< show coordinates, if possible
	Int32x2		origin;			///< rotation origin for polar conversion, ray origin for psrays
	Int32x2		pageOrigin;		///< output placement
	Int32x2		zoomOrigin;		///< magnifier spot
	Int32		pageZoom;		///< output zoom factor
};
typedef struct BtOutputContextSimple BtOutputContextSimple;

/** Multi-format output function.
 * @param context state and settings of output
 * @param btList the internal data representation list. This list is consumed by the output function.
 * @param outputFormat the selected output format
 * @return true for complete and successful output, false otherwise
 */
bool btOutputSimple (const BtOutputContextSimple *context, BtList *btList, int outputFormat);

#endif

