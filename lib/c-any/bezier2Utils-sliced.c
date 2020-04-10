//HEADER
#include <bezierUtils.h>
#include <bezier2Utils.h>
#include <int32Math.h>
#include <bezier.h>

#define SHOW(x0) \
		fifoPrintString (out,#x0"="); \
		fifoPrintInt32x2_en (out,e,3,x0); \
		fifoPrintString (out," = "); \
		fifoPrintInt32x2Hex (out,x0); \
		fifoPrintLn (out); \
		outFlush();

//SLICE
bool int32x2CurvePsDrawBezier2_en (Fifo *fifo, int e, const Int32x2Curve *curve) {
	Int32x2Fifo segments = *curve->segments;
	Int32x2 x0 = curve->x0;
	bool success = true;
	while (success && 2<=int32x2FifoCanRead (&segments)) {
		const Int32x2 x1 = int32x2FifoRead (&segments);
		const Int32x2 x2 = int32x2FifoRead (&segments);
		success = success && fifoPsDrawBezier2_en (fifo,e,x0,x1,x2);
		x0 = x2;
	}
	return success && !int32x2FifoCanRead (&segments);	// all written no lingering points
}

//SLICE
bool int32x2CurveBezier2DeCasteljau (Int32x2Curve *curve) {
	int n = int32x2FifoCanRead (curve->segments);
	if ((n&1)==0 && (n>=2)) {	// even number >= 2
		Int32x2 x0 = curve->x0;

		for (int i=0; i+2-1<n; i+=2) {
			if (int32x2FifoCanReadArray (curve->segments)<2) return -1;
			if (int32x2FifoCanWriteArray (curve->segments)<4) return -2;
			const Int32x2 *xs = int32x2FifoReadArrayBegin (curve->segments);
			const Int32x2 x1 = xs[0];
			const Int32x2 x2 = xs[1];
			const Int32x2 y1 = int32x2Avg (x0,x1);
			const Int32x2 z1 = int32x2Avg (x1,x2);
			const Int32x2 y2 = int32x2Avg (y1,z1);
			Int32x2 *vs = int32x2FifoWriteArrayBegin (curve->segments,4);
			vs[0] = y1;
			vs[1] = y2;
			vs[2] = z1;
			vs[3] = x2;
			int32x2FifoWriteArrayEnd (curve->segments,4);
			int32x2FifoReadArrayEnd (curve->segments,2);

			x0 = x2;
		}
		return true;
	}
	else return false;
}

//SLICE
Uint32 int32Bezier2Unflatness2_en (int e, Int32x2 x0, Int32x2 x1, Int32x2 x2) {
	const Int32x2 xm = int32x2Avg(
		int32x2Avg (x0,x1),
		int32x2Avg (x1,x2)
	);
	return int32x2Abs2_en (e,int32x2Sub (x1,xm));
}

//SLICE
Uint32 int32Bezier2Unflatness (Int32x2 x0, Int32x2 x1, Int32x2 x2) {
	const Int32x2 xm = int32x2Avg(
		int32x2Avg (x0,x1),
		int32x2Avg (x1,x2)
	);
	return int32x2Abs (int32x2Sub (x1,xm));
}

//SLICE
Uint32 int32x2CurveBezier2DeCasteljauFlattenStep_en (int e, Uint32 unflatness2_en, Int32x2Curve *curve) {
	Uint32 maxUnflatness2_en = 0;

	int n = int32x2FifoCanRead (curve->segments);	// :o) no checking of lingering point at the end...
	Int32x2 x0 = curve->x0;
	for (int i=0; i<n; i+=2) {
		const Int32x2 x1 = int32x2FifoRead (curve->segments);
		const Int32x2 x2 = int32x2FifoRead (curve->segments);
		Uint32 thisUnflatness2_en = int32Bezier2Unflatness2_en (e,x0,x1,x2);
		if (thisUnflatness2_en > unflatness2_en) {
			if (int32x2FifoCanWrite (curve->segments)<4) return -1;
			const Int32x2 y1 = int32x2Avg (x0,x1);
			const Int32x2 z1 = int32x2Avg (x1,x2);
			const Int32x2 y2 = int32x2Avg (y1,z1);
			const Int32x2 z2 = x2;
			int32x2FifoWrite (curve->segments,y1);
			int32x2FifoWrite (curve->segments,y2);
			int32x2FifoWrite (curve->segments,z1);
			int32x2FifoWrite (curve->segments,z2);
			maxUnflatness2_en = uint32Max (maxUnflatness2_en,int32Bezier2Unflatness2_en (e,x0,y1,y2));
			maxUnflatness2_en = uint32Max (maxUnflatness2_en,int32Bezier2Unflatness2_en (e,y2,z1,z2));
		}
		else {	// already flat enough
			int32x2FifoWrite (curve->segments,x1);
			int32x2FifoWrite (curve->segments,x2);
			maxUnflatness2_en = uint32Max (maxUnflatness2_en,thisUnflatness2_en);
		}

		x0 = x2;	// for next iteration
	}
	return maxUnflatness2_en;
}

//SLICE
Uint32 int32x2CurveBezier2LimitLengthDeCasteljauStep_en (int e, Int32 maxLength_en, Int32x2Curve *curve) {
	Uint32 max_en = 0;

	int n = int32x2FifoCanRead (curve->segments);	// :o) no checking of lingering point at the end...
	Int32x2 x0 = curve->x0;
	for (int i=0; i<n; i+=2) {
		const Int32x2 x1 = int32x2FifoRead (curve->segments);
		const Int32x2 x2 = int32x2FifoRead (curve->segments);
		const Int32 thisLength_en = int32x2Abs (int32x2Sub (x2,x0));
		if (i==0) max_en = thisLength_en;

		if (thisLength_en > maxLength_en) {
			if (int32x2FifoCanWrite (curve->segments)<4) return -1;
			const Int32x2 y1 = int32x2Avg (x0,x1);
			const Int32x2 z1 = int32x2Avg (x1,x2);
			const Int32x2 y2 = int32x2Avg (y1,z1);
			const Int32x2 z2 = x2;
			int32x2FifoWrite (curve->segments,y1);
			int32x2FifoWrite (curve->segments,y2);
			int32x2FifoWrite (curve->segments,z1);
			int32x2FifoWrite (curve->segments,z2);
			max_en = uint32Max (max_en,int32x2Dist (x0,y2));
			max_en = uint32Max (max_en,int32x2Dist (y2,x2));
		}
		else {	// already flat enough
			int32x2FifoWrite (curve->segments,x1);
			int32x2FifoWrite (curve->segments,x2);
			max_en = uint32Max (max_en,thisLength_en);
		}

		x0 = x2;	// for next iteration
	}
	return max_en;
}

//SLICE
int int32x2CurveBezier2ToolRadius_en (int e, Int32 r_en, Int32x2 x_1, Int32x2Curve *curve) {
	// x_1 and x0 define a safe path to x0
	Int32x2 x0 = curve->x0;
	Int32x2 y0 = int32x2Add (x0, int32x2ToolOffset_en (e,r_en,x_1,x0));
	curve->x0 = y0;

	int segments = 0;
	const int n = int32x2FifoCanRead (curve->segments);
	for (int i=0; i<n/2; i++) {
		Int32x2 x1 = int32x2FifoRead (curve->segments);
		Int32x2 x2 = int32x2FifoRead (curve->segments);
		segments += int32x2Bezier2ToolRadiusStep_en (e,r_en,x_1,x0,x1,x2,curve->segments);
		x_1 = x1;
		x0 = x2;
	}
	return segments;
}

//SLICE
int int32x2CurveBezier2ToolRadiusOptimistic_en (int e, Int32 r_en, Int32x2Curve *curve) {
	// Start position is x0. The path to x0 is assumed to be done already in a safe way,

	const int n = int32x2FifoCanRead (curve->segments);
	int segments = 0;
	Int32x2 x0 = curve->x0;
	if (n >= 2) {
		Int32x2 h = int32x2FifoLookAhead (curve->segments,0);
		Int32x2 x_1 = int32x2Sub (x0,int32x2Sub (h,x0));	// extrapolation back
		Int32x2 y0 = int32x2Add (x0, int32x2ToolOffset_en (e,r_en,x_1,x0));
		// Add the starting point...
		curve->x0 = y0;

		for (int i=0; i<n/2; i++) {
			Int32x2 x1 = int32x2FifoRead (curve->segments);
			Int32x2 x2 = int32x2FifoRead (curve->segments);
			segments += int32x2Bezier2ToolRadiusStep_en (e,r_en,x_1,x0,x1,x2,curve->segments);
			x_1 = x1;
			x0 = x2;
		}
		return segments;
	}
	else return 0;
}

//SLICE
int int32x2Bezier2ToolRadiusStep_en (int e, Int32 r_en, Int32x2 x_1, Int32x2 x0, Int32x2 x1, Int32x2 x2, Int32x2Fifo *curve) {
	int segments = 1;
	if (curve!=0 && int32x2FifoCanWrite (curve)<5) return 0;

	const Int32x2 r_1 = int32x2ToolOffset_en (e,r_en,x_1,x0);
	const Int32x2 r0  = int32x2ToolOffset_en (e,r_en,x0,x1);
	const Int32x2 r2  = int32x2ToolOffset_en (e,r_en,x1,x2);

	const Int32x2 y0 = int32x2Add (x0,r0);
	const Int32x2 y2 = int32x2Add (x2,r2);

#if DEBUG_TOOLRADIUS==1
static bool first = true;
if (first) {
	first = false;
	SHOW(x_1)
	SHOW(x0)
	SHOW(x1)
	SHOW(x2)
	SHOW(y_1)
	SHOW(y0)
	SHOW(y2)
	SHOW(r_1)
	SHOW(r0)
	SHOW(r2)
}

/*
	const Int32x2 O = { };

	printf("yellow\n");
	psAtVector ("x_1",O,x_1);
	psAtVector ("x0",O,x0);
	psAtVector ("x1",O,x1);
	psAtVector ("x2",O,x2);
*/
	fifoPrintStringLn (out,"black");
	fifoPsDrawComplexFromTo_en (out,e,x_1,x0,0);
	fifoPsDrawComplexFromTo_en (out,e,x0,x1,"0->1");
	fifoPsDrawComplexFromTo_en (out,e,x1,x2,"1->2");
	fifoPsDrawBezier2_en (out,e,x0,x1,x2);

	fifoPrintStringLn (out,"green");
	fifoPsDrawComplex_en (out,e,x0,r_1,"r_1");
	fifoPsDrawComplex_en (out,e,x0,r0,"r0");
	fifoPsDrawComplex_en (out,e,x1,r0,"r0");
	fifoPsDrawComplex_en (out,e,x2,r2,"r2");
	fifoPsDrawComplex_en (out,e,x1,r2,"r2");

#endif
	// do we need a bridge-segment?
	// yes, if yn1 and y0 have a measurable distance
//	const Uint32 dist_en = int32x2Dist (r_1,r0);
	const Uint32 dist_en = int32x2Dist (r_1,r0);
	if (dist_en >= 1u<<e/2) {	// rough estimate... :o)
		segments = 2;
		const Int32x2 m = int32x2Add (x0, int32x2Miter_en (e, r_1,r0));
#if DEBUG_TOOLRADIUS==1
		fifoPrintStringLn (out,"magenta");
		fifoPsDrawComplexFromTo_en (out,e,x0,m,"bm");
		fifoPsDrawBezier2_en (out,e,y_1,m,y0);
#endif
		if (curve!=0) {
			int32x2FifoWrite (curve,m);
			int32x2FifoWrite (curve,y0);	// replaces yn1 as starting point for 2nd segment
		}
	}

	// normal segment. Miter join
	const Int32x2 y1 = int32x2Add (x1, int32x2Miter_en (e, r0,r2));
#if DEBUG_TOOLRADIUS==1
	fifoPrintStringLn (out,"green");
	fifoPsDrawComplexFromTo_en (out,e,x1,y1,"m");
	fifoPsDrawBezier2_en (out,e,y0,y1,y2);
	outFlush();
	errFlush();
#endif
	// store the translated segment
	if (curve!=0) {
		int32x2FifoWrite (curve,y1);
		int32x2FifoWrite (curve,y2);
	}

	// next iteration
	// x_1 = x1;
	// x0 = x2;
	// y_1 = y2;

	return segments;
}

//SLICE
bool int32x2CurveBezier2ClosePathAt (int e, Int32 excentricity_en, Int32x2 cp, Int32x2Curve *curve) {
	const int r = int32x2FifoCanRead (curve->segments);
	const int w = int32x2FifoCanWrite (curve->segments);

	if (r>=2 && w>=4) {
		// initial tangent definition
		const Int32x2 x0 = curve->x0;
		const Int32x2 x1 = int32x2FifoLookAhead (curve->segments,0);
		// final tangent definition
		const Int32x2 y1 = int32x2FifoLookAhead (curve->segments,-2);
		const Int32x2 y2 = int32x2FifoLookAhead (curve->segments,-1);
		
		const Int32 sineLimit_en = 1<<e-8;
		const Int32x2 s = int32x2IntersectLineLineBetween_en (e,sineLimit_en, x0,int32x2Sub(x1,x0), y2,int32x2Sub(y1,y2));

		const Int32x2 z1 = int32x2Add (y2, int32x2Scale_en (e,int32x2Sub (s,y2), excentricity_en));
		const Int32x2 z3 = int32x2Add (x0, int32x2Scale_en (e,int32x2Sub (s,x0), excentricity_en));

		// connecting segments
		int32x2FifoWrite (curve->segments,z1);
		int32x2FifoWrite (curve->segments,cp);	// end point

		curve->x0 = cp;
		int32x2FifoWrite (curve->segments,z3);
		int32x2FifoWrite (curve->segments,x0);
		int32x2FifoRotate (curve->segments,-2);
		return true;
	}
	else return false;
}

