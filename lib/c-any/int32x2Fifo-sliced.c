//HEADER
#include <int32x2Fifo.h>

inline static int imin(int a, int b) { return a<b ? a : b; }
inline static int imax(int a, int b) { return a>b ? a : b; }

inline static int int32x2FifoFloss (const Int32x2Fifo *ci) {
	return ci->wFragment - ci->rFragment;
}

//SLICE
Int32x2 int32x2FifoAvg (const Int32x2Fifo *points) {
	Int32x2Fifo clone = *points;

	Int64 sumX=0;
	Int64 sumY=0;
	int n=0;

	while (int32x2FifoCanRead (&clone)) {
		const Int32x2 p = int32x2FifoRead (&clone);
		sumX += p.x;
		sumY += p.y;
		n++;
	}

	Int32x2 avg = {
		sumX / n,
		sumY / n
	};
	return  avg;
}


//SLICE
int int32x2FifoCanRead (const Int32x2Fifo *ci) {
	return ci->wTotal - ci->rTotal;
}

//SLICE
int int32x2FifoCanWrite (const Int32x2Fifo *ci) {
	return ci->size/(Int32)sizeof(Int32x2) - ci->wTotal + ci->rTotal - int32x2FifoFloss (ci);	// sizeof(Int32x2)==8 which is a power of 2 :-)
}

//SLICE
Int32x2 int32x2FifoRead (Int32x2Fifo *ci) {
	Int32x2 const *px = ci->xs+ci->rPos;
	ci->rPos++; if (ci->rPos >= ci->size/sizeof(Int32x2) - int32x2FifoFloss (ci)) {
		ci->rPos = 0;
		ci->rFragment = ci->wFragment;	// read across end zeros the fragmentation
	}
	ci->rTotal++;
	return *px;
}

//SLICE
Int32x2 int32x2FifoLookAhead (const Int32x2Fifo *ci, int n) {
	if (n<0) n = int32x2FifoCanRead (ci) + n;	// handle negative values that don't exceed the size
	const int limit = ci->size/sizeof(Int32x2) - int32x2FifoFloss (ci);
	const int rPos =  ci->rPos+n >= limit ? ci->rPos+n - limit : ci->rPos+n;
	return ci->xs [rPos];
}

//SLICE
void int32x2FifoWrite (Int32x2Fifo *ci, Int32x2 e) {
	ci->xs[ci->wPos] = e;
	ci->wPos++; if (ci->wPos >= ci->size/sizeof(Int32x2)) ci->wPos = 0;
	ci->wTotal++;
}

//SLICE
int int32x2FifoCanReadArray (const Int32x2Fifo *ci) {
	const int toEnd = ci->size/sizeof(Int32x2) - ci->rPos - int32x2FifoFloss (ci);	// take care of discarded elements
	const int readable = int32x2FifoCanRead (ci);
	return imin (toEnd,readable);
}

//SLICE
int int32x2FifoCanWriteArray (const Int32x2Fifo *ci) {
	const int writeable = int32x2FifoCanWrite (ci);
	const int toEnd = ci->size/sizeof(Int32x2) - ci->wPos;
	if (writeable!=0) {	// writeable!=0 ==> ci->wPos != ci->rPos
		return ci->rPos < ci->wPos	// no fragment, >0 chars to read
		? imax (ci->rPos,toEnd)		// free cells .   ...r##w.....  
		: writeable;			// free cells .   ###w..r####*  *=fragment
	}
	else return 0;
}

//SLICE
const Int32x2* int32x2FifoReadArrayBegin (const Int32x2Fifo *ci) {
	return  ci->xs+ci->rPos;
}

//SLICE
void int32x2FifoReadArrayEnd (Int32x2Fifo *ci, int n) {
	ci->rPos += n;
	const int wrapPosition = ci->size/sizeof(Int32x2) - int32x2FifoFloss (ci);
	if (ci->rPos >= wrapPosition) {
		ci->rPos -= wrapPosition;
		ci->rFragment = ci->wFragment;	// read across end zeros the fragmentation
	}
	ci->rTotal += n;
}

//SLICE
Int32x2* int32x2FifoWriteArrayBegin (Int32x2Fifo *ci, int length) {
	const int toEnd = ci->size/(Int32)sizeof(Int32x2) - ci->wPos;				// sizeof(Int32x2)==8 which is a power of 2 :-)

	if (toEnd>=length);	// all fine, no fragmentation, yet
	else {	// discard the end of the buffer
		ci->wFragment += toEnd;
		ci->wPos = 0;
	}
	return ci->xs+ci->wPos;
}

//SLICE
void int32x2FifoWriteArrayEnd (Int32x2Fifo *ci, int length) {
	ci->wPos += length; if (ci->wPos*sizeof(Int32x2)>=ci->size) ci->wPos = 0;
	ci->wTotal += length;
}

//SLICE
void int32x2FifoMap (Int32x2Fifo *fifo, Int32x2 (*f)(Int32x2)) {
	for (int i=int32x2FifoCanRead (fifo); i>0; i--) int32x2FifoWrite(fifo, f(int32x2FifoRead(fifo)) );
}

//SLICE
void int32x2CurveMap (Int32x2Curve *curve, Int32x2 (*f)(Int32x2)) {
	curve->x0 = f(curve->x0);
	for (int i=int32x2FifoCanRead (curve->segments); i>0; i--) int32x2FifoWrite(curve->segments, f(int32x2FifoRead(curve->segments)) );
}

/*
void int32x2FifoDump (const char *s, Int32x2Fifo const *ci) {
	Int32x2Fifo it = *ci;

	printf ("%s:",s);
	printf ("wPos=%2d,rPos=%2d,wTotal=%2d,rTotal=%2d,wFrag=%2d,rFrag=%2d : ",ci->wPos,ci->rPos,ci->wTotal,ci->rTotal,ci->wFragment,ci->rFragment);
	while (int32x2FifoCanRead(&it)) {
		const Int32x2 p = int32x2FifoRead(&it);
		printf ("(0x%x 0x%x),",p.x,p.y); 
	}
	printf ("\n");
}
*/

//SLICE
void int32x2FifoRotate (Int32x2Fifo *fifo, int n) {
	if (n>=0) for (int i=0; i<n; i++) int32x2FifoWrite (fifo, int32x2FifoRead (fifo));
	else int32x2FifoRotate (fifo, int32x2FifoCanRead(fifo)+n);
}

//SLICE
void int32x2CurveRotate (Int32x2Curve *curve, int n) {
	if (int32x2FifoCanRead (curve->segments) > 0) {
		int32x2FifoRotate (curve->segments,n);
		curve->x0 = int32x2FifoLookAhead (curve->segments,-1);		// fetch the last point and use is as start.
	}
	else ;	// don't change. Curve is a single point
}

//SLICE
void int32x2FifoReverse (Int32x2Fifo *fifo) {
	const int n = int32x2FifoCanRead (fifo);
	if (n>=0) {
		Int32x2 list[n];
		for (int i=0; i<n; i++) list[i] = int32x2FifoRead (fifo);
		for (int i=n-1; i>=0; i--) int32x2FifoWrite (fifo,list[i]);
	}
}

//SLICE
void int32x2CurveReverse (Int32x2Curve *curve) {
	if (int32x2FifoCanRead (curve->segments) > 0) {
		int32x2FifoReverse (curve->segments);
		const Int32x2 newStart = int32x2FifoRead (curve->segments);
		int32x2FifoWrite (curve->segments, curve->x0);		// the new 'last one'
		curve->x0 = newStart;
	}
	else ;	// don't change. Curve is a single point
}

//SLICE
void int32x2FifoMapTranslate (Int32x2Fifo *fifo, const Int32x2 trans) {
	for (int i=int32x2FifoCanRead (fifo); i>0; i--) int32x2FifoWrite(fifo, int32x2Add (int32x2FifoRead(fifo),trans) );
}

//SLICE
void int32x2FifoMapMatrix_en (int e, Int32x2Fifo *fifo, const Int32x2x2 matrix) {
	for (int i=int32x2FifoCanRead (fifo); i>0; i--) int32x2FifoWrite(fifo, int32x2x2MulInt32x2_en (e,matrix,int32x2FifoRead(fifo)) );
}

//SLICE
void int32x2FifoMapAffine_en (int e, Int32x2Fifo *fifo, const Int32x2Affine affine) {
	for (int i=int32x2FifoCanRead (fifo); i>0; i--) int32x2FifoWrite(fifo, int32x2Affine_en (e,affine,int32x2FifoRead(fifo)) );
}


//SLICE
void int32x2CurveMapTranslate (Int32x2Curve *curve, const Int32x2 trans) {
	curve->x0 = int32x2Add (curve->x0,trans);
	int32x2FifoMapTranslate (curve->segments,trans);
}

//SLICE
void int32x2CurveMapMatrix_en (int e, Int32x2Curve *curve, const Int32x2x2 matrix) {
	curve->x0 = int32x2x2MulInt32x2_en (e,matrix,curve->x0);
	int32x2FifoMapMatrix_en (e,curve->segments,matrix);
}

//SLICE
void int32x2CurveMapAffine_en (int e, Int32x2Curve *curve, const Int32x2Affine affine) {
	curve->x0 = int32x2Affine_en (e,affine,curve->x0);
	int32x2FifoMapAffine_en (e,curve->segments,affine);
}

