//HEADER
#include <bezierUtils.h>
#include <int32Math.h>
#include <bezier.h>

//SLICE
bool int32x2CurvePsDrawPolygon_en (Fifo *fifo, int e, const Int32x2Curve *curve) {
	Int32x2Fifo points = *curve->segments;

	Int32x2 q = curve->x0;
	bool success = true;
	while (success && int32x2FifoCanRead (&points)) {
		const Int32x2 p = int32x2FifoRead (&points);
		success = success && fifoPsDrawComplexSegmentFromTo_en (fifo,e,q,p,0) && fifoPrintLn (fifo);
		q = p;
	}
	return success;
}

//SLICE
bool int32x2FifoPsDrawPolygon_en (Fifo *fifo, int e, const Int32x2Fifo *curve) {
	Int32x2Fifo points = *curve;

	if (int32x2FifoCanRead (&points)) {
		Int32x2 x0 = int32x2FifoRead (&points); 
		Int32x2 q = x0; 
		bool success = true;
		while (success && int32x2FifoCanRead (&points)) {
			const Int32x2 p = int32x2FifoRead (&points);
			success = success && fifoPsDrawComplexSegmentFromTo_en (fifo,e,q,p,0) && fifoPrintLn (fifo);
			q = p;
		}
		if (!int32x2Eq (q,x0)) {	// add a closing line
			success = success && fifoPsDrawComplexSegmentFromTo_en (fifo,e,q,x0,0) && fifoPrintLn (fifo);
		}
		return success;
	}
	else return true;	// empty polygon always prints successfully
}

//SLICE
bool int32x2CurvePsDrawRays_en (Fifo *fifo, int e, Int32x2 origin, const Int32x2Curve *curve) {
	bool success = fifoPsDrawComplexFromTo_en (fifo,e,origin,curve->x0,0);

	Int32x2Fifo clone = *curve->segments;
	while (success && int32x2FifoCanRead (&clone)) {
		const Int32x2 x = int32x2FifoRead (&clone);
		success = success && fifoPsDrawComplexFromTo_en (fifo,e,origin,x,0);
	}
	return success;
}

//SLICE
bool int32x2CurvePsDrawSunRays_en (Fifo *fifo, int e, Int32x2 origin, Int32 rSun, const Int32x2Curve *curve) {
	const Int32x2 o = int32x2Add (origin, int32x2ScaleTo_en (e, int32x2Sub (curve->x0,origin), rSun));
	bool success = fifoPsDrawComplexCircle_en (fifo,e,origin,rSun)
		&& fifoPsDrawComplexFromTo_en (fifo,e,o,curve->x0,0);
	
	Int32x2Fifo clone = *curve->segments;
	while (success && int32x2FifoCanRead (&clone)) {
		const Int32x2 x = int32x2FifoRead (&clone);
		const Int32x2 o = int32x2Add (origin, int32x2ScaleTo_en (e, int32x2Sub (x,origin), rSun));
		success = success && fifoPsDrawComplexFromTo_en (fifo,e,o,x,0);
	}
	return success;
}

//SLICE
bool int32x2FifoPsDrawRays_en (Fifo *fifo, int e, Int32x2 origin, const Int32x2Fifo *curve) {
	bool success = true;
	Int32x2Fifo clone = *curve;
	while (success && int32x2FifoCanRead (&clone)) {
		const Int32x2 x = int32x2FifoRead (&clone);
		success = success && fifoPsDrawComplexFromTo_en (fifo,e,origin,x,0);
	}
	return success;
}



//SLICE
Int32x2 int32x2CurveAvg (const Int32x2Curve *curve) {
	Int32x2Fifo clone = *curve->segments;

	Int64 sumX = curve->x0.x;
	Int64 sumY = curve->x0.y;
	int n=1;

	while (int32x2FifoCanRead (&clone)) {
		const Int32x2 p = int32x2FifoRead (&clone);
		if (int32x2FifoCanRead (&clone) || !int32x2Eq (p,curve->x0)) {
			sumX += p.x;
			sumY += p.y;
			n++;
		}
	}

	Int32x2 avg = {
		sumX / n,
		sumY / n
	};
	return  avg;
}

//SLICE
Int32x2 int32x2CurveCog (const Int32x2Curve *curve) {
	Int32x2Fifo clone = *curve->segments;

	Int64 sumX = 0;
	Int64 sumY = 0;
	Int64 sumM = 0;

	Int32x2 x0 = curve->x0;
	while (int32x2FifoCanRead (&clone)) {
		const Int32x2 p = int32x2FifoRead (&clone);
		if (int32x2FifoCanRead (&clone)) {
			const Int32x2 x = int32x2Avg (x0,p);
			const Int32 m =  int32x2Dist (x0,p);		// straight line approximation of line length
			sumM += m;
			sumX += (Int64)m * x.x;
			sumY += (Int64)m * x.y;
		}
		x0 = p;
	}

	if (sumM > 0) {
		Int32x2 avg = {
			int64Div (sumX,sumM),
			int64Div (sumY,sumM)
		};
		return  avg;
	}
	else return curve->x0;
}

//SLICE
Int32x2 int32x2Miter_en (int e, Int32x2 a, Int32x2 b) {
	const Int32x2 s = int32x2Add (a,b);
	const Int32x2 s0 = int32x2ScaleTo1_en (e,s);
	const Uint32 absA_en = int32x2Abs (a);
	const Uint32 absB_en = int32x2Abs (b);
	const Int64 absaAbsb_e2n = (Int64)absA_en*absB_en;
	const Uint32 l_en = uint32SqrtFloor_en (e,
		uint64Div (
			absaAbsb_e2n*2, (absaAbsb_e2n+int32x2MulScalar_e2n (a,b)) >> e
		)
	);
	return int32x2Scale_en (e,int32x2Scale_en (e,s0,l_en), (absA_en+absB_en)/2);
}

//SLICE
Int32x2 int32x2ToolOffset_en (int e, Int32 r_en, Int32x2 x0, Int32x2 x1) {
	const Int32x2 v00 = int32x2ScaleTo1_en (e, int32x2Sub (x1,x0));
	return int32x2Scale_en (e, int32x2j (v00), r_en);
}

//SLICE
bool int32x2CurveBezier3DeCasteljau (Int32x2Curve *curve) {
	enum { N=3 };
	int n = int32x2FifoCanRead (curve->segments);
	if (n==0) return true;

	// n!=0
	if (n<=int32x2FifoCanWrite (curve->segments)) {	// sufficient space available
		Int32x2 x0 = curve->x0;

		for (int i=0; i+N<=n; i+=N) {
			const Int32x2 x1 = int32x2FifoRead (curve->segments);
			const Int32x2 x2 = int32x2FifoRead (curve->segments);
			const Int32x2 x3 = int32x2FifoRead (curve->segments);

			const Int32x2 y1 = int32x2Avg (x0,x1);
			const Int32x2 z2 = int32x2Avg (x2,x3);
			const Int32x2 x12 = int32x2Avg (x1,x2);

			const Int32x2 y2 = int32x2Avg (y1,x12);
			const Int32x2 z1 = int32x2Avg (x12,z2);

			const Int32x2 y3 = int32x2Avg (y2,z1);
			
			int32x2FifoWrite (curve->segments,y1);
			int32x2FifoWrite (curve->segments,y2);
			int32x2FifoWrite (curve->segments,y3);	// == z0
			int32x2FifoWrite (curve->segments,z1);
			int32x2FifoWrite (curve->segments,z2);
			int32x2FifoWrite (curve->segments,x3);

			x0 = x3;
		}
		return true;
	}
	else return false;
}

//SLICE
bool int32x2CurveBezier4DeCasteljau (Int32x2Curve *curve) {
	enum { N=4 };
	int n = int32x2FifoCanRead (curve->segments);
	if (n==0) return true;

	// n!=0
	if (n <= int32x2FifoCanWrite (curve->segments)) {	// sufficient space available
		Int32x2 x0 = curve->x0;

		for (int i=0; i+N-1<n; i+=N) {
			const Int32x2 x1 = int32x2FifoRead (curve->segments);
			const Int32x2 x2 = int32x2FifoRead (curve->segments);
			const Int32x2 x3 = int32x2FifoRead (curve->segments);
			const Int32x2 x4 = int32x2FifoRead (curve->segments);

			const Int32x2 y1 = int32x2Avg (x0,x1);
			const Int32x2 z3 = int32x2Avg (x3,x4);
			const Int32x2 x12 = int32x2Avg (x1,x2);
			const Int32x2 x23 = int32x2Avg (x2,x3);

			const Int32x2 y2 = int32x2Avg (y1,x12);
			const Int32x2 z2 = int32x2Avg (x23,z3);

			const Int32x2 x123 = int32x2Avg (x12,x23);
			const Int32x2 y3 = int32x2Avg (y2,x123);
			const Int32x2 z1 = int32x2Avg (x123,z2);

			const Int32x2 y4 = int32x2Avg (y3,z1);
			
			int32x2FifoWrite (curve->segments,y1);
			int32x2FifoWrite (curve->segments,y2);
			int32x2FifoWrite (curve->segments,y3);
			int32x2FifoWrite (curve->segments,y4);	// == z0
			int32x2FifoWrite (curve->segments,z1);
			int32x2FifoWrite (curve->segments,z2);
			int32x2FifoWrite (curve->segments,z3);
			int32x2FifoWrite (curve->segments,x4);

			x0 = x4;
		}
		return true;
	}
	else return false;
}

