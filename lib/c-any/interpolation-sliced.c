//HEADER
#include <interpolation.h>
#include <int32Math.h>
#include <int64Math.h>
#include <fifoPrint.h>

#define DEBUG(x)

DEBUG(
	static const double e32 = (double)(1ll<<32);
)

// the formulas marked with (**) must be equal !!!!
//

//SLICE
int movementClock(Movement *m) {
	m->t ++;
	if (m->t<=0) return m->s;
	else if (m->t <= m->t1) {
		m->v_e20 = int32MulExp2(m->dir * m->t,20 + m->a_p);
		m->s = m->s0 + m->dir * ( (Uint64)m->t * m->t >> (1 - m->a_p) ); 	// (**)
	}
	else if (m->t < m->t2) {	// linear
		// speed stays constant here.
		m->s += fractionalCounter(&m->linear);
	}
	else if (m->t < m->t3) {
		const int t = m->t3 - m->t;
		m->v_e20 = int32MulExp2(m->dir * t,20 + m->a_p);
		m->s  = m->s3 - m->dir * ( (Uint64)t * t >> (1 - m->a_p) );		// (**)
	}
	else if (m->t < m->t4);	// pause
	else m->active = false;	// thread safe stop indication

	return m->s;
}

//SLICE
void movementInit(Movement *m, int sCurrent) {
	m->active = false;
	m->s = sCurrent;
}

//SLICE
void movementStart(Movement *m, MovementConfiguration const *mc, int s3) {
	const int s0 = m->s;	// current position
	//const unsigned tMax = mc->vMax_e32>>(32+mc->a_p);	// single steps
	const unsigned tMax = int32MulExp2(mc->vMax_e20, -20-mc->a_p);	// single steps
	const int _sAMax = (Uint64)tMax*tMax >> (1 - mc->a_p);	// 1/2a*t^2		// (**)

	const int sA = s3-s0;
	const int dir = sA>=0 ?  1 : -1;

	const unsigned _sA = sA >= 0 ? sA : -sA;
DEBUG(
	fprintf(stderr,"maximum acceleration time=%d\n",tMax);
	fprintf(stderr,"maximum acceleration=%f\n",(1<<(32+mc->a_p))/e32);
	fprintf(stderr,"maximum speed=%f\n",(double)(tMax)/(1<<(0 - mc->a_p)));
)
	if (_sA >= 4*_sAMax) {	// movement at maximum speed
		DEBUG( fprintf(stderr,"Maximum speed movement.\n"); )
		m->t1 = tMax;
		const int _sConstant = _sA - 2*_sAMax;
		const int tConstant = uint64Div((Uint64)_sConstant<<20,mc->vMax_e20);
		fractionalCounterInit(&m->linear,tConstant,_sConstant*dir);
		m->t2 = m->t1 + tConstant;
		m->t3 = m->t2 + tMax;

		m->s0 = s0;
		m->s3 = s3;

		m->a_p = mc->a_p;
		m->dir = dir;
	}
	else {		// speed is limited through distance
		DEBUG( fprintf(stderr,"Speed limited by distance.\n"); )
		const unsigned t2 = _sA << (- mc->a_p) >>1;	// 2*(s/4)/a
		const int tA = uint32SqrtFloor(t2);	// may be off by one, if not bottom
		const int _sAShort = (Uint64)tA*tA >> (1- mc->a_p);	// 1/2a*t^2 	(**)
		m->t1 = tA;
		const int _sConstant = _sA - 2*_sAShort;
		const Int64 v_e32 = (Int64)tA<<(32 + mc->a_p);	// current speed
		DEBUG( fprintf(stderr,"  peak speed=%f\n",v_e32/e32);)
		const int tConstant = uint64Div((Uint64)_sConstant<<32,v_e32);
		fractionalCounterInit(&m->linear,tConstant,_sConstant*dir);
		DEBUG( fprintf(stderr,"  tConstant=%d, sConstant=%d\n",tConstant,_sConstant*dir); )
		m->t2 = m->t1 + tConstant;
		m->t3 = m->t2 + tA;

		m->s0 = s0;
		m->s3 = s3;

		m->a_p = mc->a_p;
		m->dir = dir;
	}

	m->t = 0;
	m->s = s0;
	m->v_e20 = 0;
	m->t4 = m->t3 + mc->tPause;
	m->active = true;	// thread safe start

	DEBUG(
	fprintf(stderr,"  t0=%d\ts0=%d\n",0,m->s0);
	fprintf(stderr,"  t1=%d\n",m->t1);
	fprintf(stderr,"  t2=%d\n",m->t2);
	fprintf(stderr,"  t3=%d\ts3=%d\n",m->t3,m->s3);
	fprintf(stderr,"  t4=%d\t\n",m->t4);
	)
}

//SLICE
void movementExit(Movement *m) {
	if (m->t < m->t3) {
		m->t = m->t3;
		m->s = m->s3;
		m->v_e20 = 0;
	}
}

//SLICE
void movementFreeze(Movement *m) {
	if (m->t < m->t3) {
		m->t = m->t3;
		m->v_e20 = 0;
	}
}

//SLICE
Int32 int32SfLinearInterpolate(const Int32Sf *pf, Int32 x) {
	if (x <= pf->points[0].x) return pf->points[0].y;
	else if (x >= pf->points[pf->elements-1].x) return pf->points[pf->elements-1].y;
	else {
		for (int i=1; i<pf->elements; ++i) if (x<=pf->points[i].x) return int32LinearInterpolate(
			pf->points[i-1].x, pf->points[i-1].y,
			pf->points[i].x, pf->points[i].y,
			x
		);
		return 0;	// cannot happen.
	}
}




//SLICE
Int32 int32FsfLinearInterpolateFast(const Int32Fsf *fsf, Int32 x) {
	const int i = int32MathDiv((x - fsf->origin), fsf->sampleDistance);
	if (i<0) return fsf->samples[0];
	else if (i>=fsf->elements-1) return fsf->samples[fsf->elements-1];
	else {
		const int dx = x - i*fsf->sampleDistance;
		return int32LinearInterpolateFast(
			0,fsf->samples[i],
			fsf->sampleDistance,fsf->samples[i+1],
			dx);
	}
}

//SLICE
Int32 int32FsfLinearInterpolate(const Int32Fsf *fsf, Int32 x) {
	const int i = int32MathDiv((x - fsf->origin), fsf->sampleDistance);
	//const int n = fsf->size / sizeof *fsf->samples;
	if (i<0) return fsf->samples[0];
	else if (i>=fsf->elements-1) return fsf->samples[fsf->elements-1];
	else {
		const int dx = x - i*fsf->sampleDistance;
		return int32LinearInterpolate(
			0,fsf->samples[i],
			fsf->sampleDistance,fsf->samples[i+1],
			dx);
	}
}

//SLICE
bool int32FsfFromInt32Sf(Int32Fsf *fsf, Int32* samples, int bufferSize, const Int32Sf *sf) {
	const int n = bufferSize / sizeof *samples;
	if (n<2) return false;

	const Int32Point pL = int32SfLeft(sf);
	const Int32Point pR = int32SfRight(sf);
	fsf->samples = samples;
	fsf->elements = n;
	fsf->origin = pL.x;
	const int deltaX = pR.x - pL.x;
	fsf->sampleDistance = (deltaX+n-1 -1) / (n-1);
	for (int s=0; s<n; s++) {
		const int x = fsf->origin + s*fsf->sampleDistance;
		fsf->samples[s] = int32SfLinearInterpolate(sf,x);
	}
	return true;
}

//SLICE
bool int32UsfFromInt32Sf(Int32Usf *usf, Int32* samples, int bufferSize, const Int32Sf *sf) {
	usf->samples = samples;
	usf->elements = bufferSize / sizeof *samples;
	const Int32Point pL = int32SfLeft(sf);
	const Int32Point pR = int32SfRight(sf);
	usf->origin = pL.x;
	const int deltaX = pR.x - pL.x;
	int shift=0;
	while ((usf->elements-1 <<shift) < deltaX) shift++;
	usf->shift = shift;
	return int32UsfSampleInt32Sf(usf,sf);
}

//SLICE
bool int32UsfSampleInt32Sf(Int32Usf *usf, const Int32Sf *sf) {
	for (int s=0; s<usf->elements; s++) {
		const int x = usf->origin + s<<usf->shift;
		usf->samples[s] = int32SfLinearInterpolate(sf,x);
	}
	// Check for successful oversampling:
	for (int s=0; s<sf->elements-1; ++s) {
		// every two original samples must have a fast sample in between...
		const Int32Point pL = int32SfPoint(sf,s);
		const Int32Point pR = int32SfPoint(sf,s+1);
		if (int32UsfIndex(usf,pL.x)==int32UsfIndex(usf,pR.x)) return false;
	}
	return true;
}

//SLICE
Int32 int32UsfLinearInterpolateFast(const Int32Usf *usf, Int32 x) {
	const Int32 dx = x - usf->origin;
	const Int32 i = dx >> usf->shift;
	if (i<0) return usf->samples[0];
	else if (i>=usf->elements-1) return usf->samples[usf->elements-1];
	else {
		const int relX = dx & (1<<usf->shift)-1;
		return int32LinearInterpolateFast(
				0,usf->samples[i],
				1<<usf->shift,usf->samples[i+1],
				relX );
	}
}

//SLICE
Int32 int32UsfLinearInterpolateOld(const Int32Usf *usf, Int32 x) {
	const Int32 dx = x - usf->origin;
	const Int32 i = dx >> usf->shift;
	if (i<0) return usf->samples[0];
	else if (i>=usf->elements-1) return usf->samples[usf->elements-1];
	else {
		const int relX = dx & (1<<usf->shift)-1;
		return int32LinearInterpolate(
				0,usf->samples[i],
				1<<usf->shift,usf->samples[i+1],
				relX );
	}
}

//SLICE
// Fast *AND* full integer width --> cool :-)
Int32 int32UsfLinearInterpolate(const Int32Usf *usf, Int32 x) {
	const Int32 dx = x - usf->origin;
	const Int32 i = dx >> usf->shift;
	if (i<0) return usf->samples[0];
	else if (i>=usf->elements-1) return usf->samples[usf->elements-1];
	else {
		const int x1 = 1<<usf->shift;
		const int relX = dx & (1<<usf->shift)-1;
		return (Int64)usf->samples[i+1]*relX + (Int64)usf->samples[i]*(x1-relX) + (x1>>1) >> usf->shift;
	}
}

//SLICE
bool fifoPrintInt32UsfSampled(Fifo *fifo, const Int32Usf *usf, int n) {
	const Int32Point l = int32UsfLeft(usf);
	const Int32Point r = int32UsfRight(usf);
	const int delta = r.x - l.x;

	bool success = true;
	for (int i=0; i<n; ++i) {
		const int x = l.x + delta*i / (n-1);
		success = success
		&& fifoPrintSDec(fifo,x,1,11,true)
		&& fifoPrintString(fifo,"->")
		&& fifoPrintSDec(fifo,int32UsfLinearInterpolate(usf,x),1,11,true)
		&& fifoPrintLn(fifo);
	}
	return success;
}

//SLICE
bool fifoPrintInt32Usf(Fifo *fifo, const Int32Usf *usf) {
	bool success =
		fifoPrintString(fifo,"Usf elements=") && fifoPrintSDec(fifo,usf->elements,1,11,false)
		&& fifoPrintString(fifo,", shift=") && fifoPrintSDec(fifo,usf->shift,1,11,false)
		&& fifoPrintString(fifo,", origin=") && fifoPrintSDec(fifo,usf->origin,1,11,false)
		&& fifoPrintString(fifo,", samples=...") && fifoPrintLn(fifo);

	for (int i=0; i<usf->elements; ++i) {
		const int x = usf->origin + (i<<usf->shift);
		success = success
		&& fifoPrintString(fifo,"  ")
		&& fifoPrintSDec(fifo,x,1,11,true)
		&& fifoPrintString(fifo,"->")
		&& fifoPrintSDec(fifo,usf->samples[i],1,11,true)
		&& fifoPrintLn(fifo);
	}
	return success;
}

