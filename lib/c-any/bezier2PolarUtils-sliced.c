//HEADER
#include <bezier2PolarUtils.h>
#include <bezier2Utils.h>
#include <int32Math.h>

//SLICE
Int32x2 int32x2PolarToCartesian (int e, int eAngle, Int32x2 p) {
	const Int32 angle_e32 = p.y<<32-eAngle;
	const Int32 r_en = p.x;

	return int32x2Scale_en (e,
		int32x2Rot_en (e,angle_e32),
		r_en
	);
}

//SLICE
bool int32x2PolarBezier2RadiusMin (Int32x2 p0, Int32x2 p1, Int32x2 p2) {
	return 0 < int32Bezier2Min (p0.x, p1.x, p2.x);
}

//SLICE
/** Duplicated 30.05.2016 from c-any/bezierUtils-sliced.c
 * Moved back to bezierUtils-sliced.c 03.06.2016
 * Moved to bezier2PolarUtils 04.07.2016
 *
 * Polar conversion of a Bezier2 segment. The curve has to be smooth (like a tool-radius compensated curve, for example).
 * The curve is assumed to keep away from the polar origin, so NEVER will negative radii be used.
 * @param e fixed point position of the vector components. Must be less or equal to 29.
 * @param eAngle the fixed point position for angular movement; 1.0 corresponds to one revolution
 * @param center the center of rotation
 * @param x0Angle_en the accumulated angle of the current segment's x0.
 * @param x0 the starting point
 * @param x1 the control point defining the tangents
 * @param x2 the end point
 * @param curve the polar coordinate curve.
 * @return true in case of success. False if a negative radius is calculated or not polar intersection was obtainable.
 */
bool int32x2PolarStep_en (int e, int eAngle, Int32x2 center, Int32 x0Angle_en, const Int32x2 xs[3], Int32x2 ps[3]) {
	const Int32x2 x0 = int32x2Sub (xs[0],center);
	const Int32x2 x1 = int32x2Sub (xs[1],center);
	const Int32x2 x2 = int32x2Sub (xs[2],center);

#if DEBUG_POLAR==1
	fifoPrintString (out,"green ");
/*	fifoPsDrawComplex_en (out,e,center,x0,"x0");
	fifoPsDrawComplex_en (out,e,center,x1,"x1");
	fifoPsDrawComplex_en (out,e,center,x2,"x2");
	fifoPsDrawBezier2_en (out,e,xs[0],xs[1],xs[2]);
*/	outFlush ();
#endif

	const Int32 r0_en = int32x2Abs (x0);
	const Int32 r2_en = int32x2Abs (x2);
	const Int32x2 c0 = int32x2ScaleTo1_en (e,x0);	// orientation x0
	const Int32x2 c1 = int32x2ScaleTo1_en (e,x1);	// orientation x1
	const Int32x2 c2 = int32x2ScaleTo1_en (e,x2);	// orientation x2
	// A Bezier2 curve cannot do more than 180 degrees. But some polar origins still see more then 180 degrees, if close to the curve.
	// Therefore, we have to take the 3rd defining point of the Bezier curve into account to determine the turning direction
	const Uint32 s0_e32 = int32x2Angle_e32 (c0);
	const Uint32 s1_e32 = int32x2Angle_e32 (c1);
	const Uint32 s2_e32 = int32x2Angle_e32 (c2);
	const bool ccw =(Uint32)(s1_e32-s0_e32)<(Uint32)(s2_e32-s0_e32);	// clockwise orientation x0 -> x1 -> x2 around center ?
	//const bool ccw = false;

#if DEBUG_POLAR==1
	static int n=0;
	n++;
	fifoPrintString (err,"Intersection calculation segment #:");
	fifoPrintInt32 (err,n,1);

	fifoPrintString (err,"\nangle0=");
	fifoPrintInt32_e20 (err,s0_e32>>12,2,6);
	fifoPrintString (err,"\nangle1=");
	fifoPrintInt32_e20 (err,s1_e32>>12,2,6);
	fifoPrintString (err,"\nangle2=");
	fifoPrintInt32_e20 (err,s2_e32>>12,2,6);
	fifoPrintLn (err);
	errFlush ();
#endif
	const Uint32 unsignedSDiff_e32 = s2_e32-s0_e32;		// range 0..1-
	const Int32 s0_en = (x0Angle_en & -1u<<eAngle) + (s0_e32 >> 32-eAngle);	// add accumulated revolutions
	const Int32 s2_en = ccw
			? s0_en + (unsignedSDiff_e32>>32-eAngle)
			: s0_en - (-unsignedSDiff_e32>>32-eAngle)	// don't simplify this formula - it is simplified already!
			;

#if DEBUG_POLAR==1
	fifoPrintString (err,"finalAngle0=");
	fifoPrintInt32_e20 (err,s0_en,2,6);
	fifoPrintString (err,"\nfinalAngle2=");
	fifoPrintInt32_e20 (err,s2_en,2,6);
	fifoPrintLn (err);
	errFlush ();
#endif
	const Int32x2 v0Rot = int32x2MulComplex_en (e,
				int32x2Scale ( int32x2Sub (x1,x0), 2),
				int32x2Conj (c0)	// inverse
			);	
	const Int32x2 v2Rot = int32x2MulComplex_en (e,
				int32x2Scale ( int32x2Sub (x1,x2), 2),
				int32x2Conj (c2)	// inverse
			);

	const Int32 w_en = PIx2_E29>>29-eAngle;

	const Int32x2 scale0 = {
		1<<e,
		int32Inverse_en (e, int32Mul_en (e,w_en,r0_en))
	};
	const Int32x2 scale2 = {
		1<<e,
		int32Inverse_en (e, int32Mul_en (e,w_en,r2_en))
	};

	const Int32x2 p0 = { r0_en, s0_en };
	const Int32x2 p2 = { r2_en, s2_en };
	const Int32x2 dir0 = int32x2ScaleXy_en (e,v0Rot,scale0);
	const Int32x2 dir2 = int32x2ScaleXy_en (e,v2Rot,scale2);

#if DEBUG_POLAR==1
	fifoPrintString (err,"p0=");
	fifoPrintInt32x2_en (err,e,6,p0);
	fifoPrintString (err,"\np2=");
	fifoPrintInt32x2_en (err,e,6,p2);
	fifoPrintString (err,"\ndir0=");
	fifoPrintInt32x2_en (err,e,6,dir0);
	fifoPrintString (err,"\ndir2=");
	fifoPrintInt32x2_en (err,e,6,dir2);
	fifoPrintLn (err);
	errFlush ();
#endif
	//const Int32 sineLimit_en = (1<<e) / 16;		// :o) needs to be well chosen!!
	//const Int32 sineLimit_en = (1<<e) / 256;		// :o) needs to be well chosen!! (this is a moderately good one)
	//const Int32 sineLimit_en = (1<<e) / 512;		// :o) needs to be well chosen!! (this is a good one)
	//const Int32 sineLimit_en = (1<<e) / 1024;		// :o) needs to be well chosen!! (this is a good one)
	const Int32 sineLimit_en = (1<<e) / 2048;		// :o) needs to be well chosen!!
	//const Int32 sineLimit_en = 0;

	Int32x2 p1 = int32x2IntersectLineLineBetween_en (e, sineLimit_en, p0,dir0, p2,dir2);

#if DEBUG_POLAR==1
	if (int32x2IsCriticalIntersectLineLine_een (e,sineLimit_en,dir0,dir2)) {
		fifoPrintStringLn (err,"Critical Intersect.");
		errFlush();
	}
#endif

	//Int32x2 p1 = int32x2IntersectLineLineBetween (e,sineLimit_en, p0,dir0, p2,dir2);
	// finally we have p1!!!!!!
	ps[0] = p0;
	ps[1] = p1;
	ps[2] = p2;

#if DEBUG_POLAR==1
	fifoPrintString (err,"p1=");
	fifoPrintInt32x2_en (err,e,6,p1);
	fifoPrintLn (err);

	if (0 >= int32x2PolarBezier2RadiusMin (p0,p1,p2)) {
		fifoPsDrawComplexMark_en (out,e,int32x2PolarToCartesian_een (e,eAngle,p1),"SUSPICIOUS: negative radius of p1");
		fifoPrintLn (out);
		outFlush ();
		fifoPrintString (err,"SUSPICIOUS: negative radius of p1: ");
		fifoPrintInt32x2_en (err,20,6,p1);
		fifoPrintLn (err);
		errFlush();
		//return false;
	}

	char buf[20];
	snprintf (buf,sizeof buf,"p#%d",n);
	Int32x2 xx0 = int32x2Add (center,int32x2PolarToCartesian_een (e,eAngle,p0));
	fifoPsDrawComplexFromTo_en (out,e,center, xx0,buf);

	int32x2DrawPolarBezier2Segment_en (out,e,eAngle,center,p0,p1,p2);
	outFlush();
#endif

	return true;
}

//SLICE
/** Translates a complete curve into polar coords.
 * @param center the rotation point
 * @param angleScale_en the value for one revolution
 * @param curve as input: cartesian curve; as output: polar curve
 * @return the number of converted segments.
 */
bool int32x2CurvePolar_en (int e, int eAngle, Int32x2 center, Int32 x0Angle_en, Int32x2Curve *curvePolar, const Int32x2Curve *curveCartesian) {
	// first two points are from save position to curve
	const int n = int32x2FifoCanRead (curveCartesian->segments);
	Int32x2Fifo segments = *curveCartesian->segments;
	if (n>=2
	&& int32x2FifoCanWrite (curvePolar->segments)>=n) {	// destination sufficiently large
		Int32x2 x0 = curveCartesian->x0;
		for (int i=0; i+1<n; i+= 2) {
			const Int32x2 xs[3] = { x0, int32x2FifoRead(&segments), int32x2FifoRead (&segments) };
			Int32x2 ps[3];
			if (!int32x2PolarStep_en (e,eAngle,center,x0Angle_en,xs,ps)) return -1;
			x0 = xs[2];

			if (i==0) curvePolar->x0 = ps[0];
			int32x2FifoWrite (curvePolar->segments,ps[1]);
			int32x2FifoWrite (curvePolar->segments,ps[2]);
			x0Angle_en = ps[2].y;		// this angle is used for the next segment
		}
		return true;
	}
	else return false;
}

//SLICE
/** Translates a complete curve into polar coords.
 * @param center the rotation point
 * @param angleScale_en the value for one revolution
 * @param curve as input: cartesian curve; as output: polar curve
 * @return the number of converted segments.
 */
int int32x2FifoPolar_en (int e, int eAngle, Int32x2 center, Int32 x0Angle_en, Int32x2Fifo *curve) {
	// first two points are from save position to curve
	const int n = int32x2FifoCanRead (curve);
	if (n>=3) {
		int segments = 0;
		Int32x2 x0 = int32x2FifoRead (curve);
		for (int i=1; i+1<n; i+= 2) {
			const Int32x2 xs[3] = { x0, int32x2FifoRead(curve), int32x2FifoRead (curve) };
			Int32x2 ps[3];
			if (!int32x2PolarStep_en (e,eAngle,center,x0Angle_en,xs,ps)) return -1;
			segments++;
			x0 = xs[2];

			if (i==1) int32x2FifoWrite (curve,ps[0]);
			int32x2FifoWrite (curve,ps[1]);
			int32x2FifoWrite (curve,ps[2]);
			x0Angle_en = ps[2].y;		// this angle is used for the next segment
		}
		return segments;
	}
	else return 0;
}

//SLICE
bool fifoPsDrawInt32x2FifoRays_en (Fifo *fifo, int e, Int32x2 origin, const Int32x2Fifo *curve) {
	bool success = true;
	Int32x2Fifo clone = *curve;
	while (success && int32x2FifoCanRead (&clone)) {
		const Int32x2 x = int32x2FifoRead (&clone);
		success = success && fifoPsDrawComplexFromTo_en (fifo,e,origin,x,0);
	}
	return success;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//SLICE
bool int32x2DrawPolarBezier2Segment_en (Fifo *fifo, int e, int eAngle, Int32x2 center, Int32x2 p0, Int32x2 p1, Int32x2 p2) {
	enum {
		DECASTELJAU_STEPS = 2,		// 2^4 segments
	};
	bool success = true
/*		&& fifoPrintString (fifo,"black ")
		&& fifoPsDrawComplex_en (fifo,e,center,int32x2PolarToCartesian_een (e,eAngle,p0),"p0")
		&& fifoPsDrawComplex_en (fifo,e,center,int32x2PolarToCartesian_een (e,eAngle,p1),"p1")
		&& fifoPsDrawComplex_en (fifo,e,center,int32x2PolarToCartesian_een (e,eAngle,p2),"p2")
*/		;

	// Use DeCasteljau flattening to get points in between
	Int32x2 buffer[3<<DECASTELJAU_STEPS];		// :o) this is way too much, but I'm fighting desperately against crazy bugs...
	Int32x2Fifo segments = { buffer, sizeof buffer, };

	Int32x2Curve curve = { p0, &segments };
	int32x2FifoWrite (curve.segments,p1);
	int32x2FifoWrite (curve.segments,p2);

	fifoPrintString (fifo,"% from polar points:");
	fifoPsPrintComplex_en (fifo,e,p0);
	fifoPsPrintComplex_en (fifo,e,p1);
	fifoPsPrintComplex_en (fifo,e,p2);
	fifoPrintLn (fifo);
	for (int c=0; c<DECASTELJAU_STEPS; c++) int32x2CurveBezier2DeCasteljau (&curve);

	Int32x2 pPrev = curve.x0;
	while (success && int32x2FifoCanRead (curve.segments)) {
		const Int32x2 p = int32x2FifoRead (curve.segments);
		success = success
//			&& fifoPrintStringLn (fifo,"red ")
//			&& fifoPsDrawComplex_en (fifo,e,center,int32x2PolarToCartesian_een (e,eAngle,p),"p");
			// draw polygon from edge to edge
//			&& fifoPrintStringLn (fifo,"blue ")
			&& fifoPsDrawComplexSegmentFromTo_en (fifo,e,
				int32x2Add (center,int32x2PolarToCartesian_een (e,eAngle,pPrev)),
				int32x2Add (center,int32x2PolarToCartesian_een (e,eAngle,p)),
				0
			);
		
		pPrev = p;
	}
	return success;
}

//SLICE
bool int32x2FifoDrawPolarBezier2Curve_en (Fifo *fifo, int e, int eAngle, Int32x2 center, const Int32x2Curve *curve) {
	bool success = true;
	Int32x2 p0 = curve->x0;
	Int32x2Fifo segments = *curve->segments;
	while (success && 2<=int32x2FifoCanRead (&segments)) {
		Int32x2 p1 = int32x2FifoRead (&segments);
		Int32x2 p2 = int32x2FifoRead (&segments);
		success = success && int32x2DrawPolarBezier2Segment_en (fifo,e,eAngle,center,p0,p1,p2);
		p0 = p2;
	}

	return success;
}

