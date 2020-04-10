//HEADER
#include <int32x2.h>
#include <int32Math.h>
#include <fifoPrintFixedPoint.h>


//SLICE
Int32x2 int32x2MulExp2 (Int32x2 a, int shl) {
	const Int32x2 r = {
		int32MulExp2 (a.x,shl),
		int32MulExp2 (a.y,shl)
	};
	return r;
}

//SLICE
Int32x2 int32x2ComplexHalfAngleQ14 (Int32x2 c) {
	const Int32x2 c2 = {
		c.x + int32x2Abs (c),
		c.y
	};
	return c.y>=0
		? c2 			// Q1 : z + |z|
		: int32x2jj (c2);	// Q4 : -z - |z|
}

//SLICE
Int32x2 int32x2ComplexHalfAngleQ23 (Int32x2 c) {
	const Int32x2 c2 = {		// -jz + j |z|
		c.y,
		-c.x + int32x2Abs (c)
	};
	return c2;
}

//SLICE
Int32x2 int32x2ComplexHalfAngle (Int32x2 c) {
	return c.x>=0 ? int32x2ComplexHalfAngleQ14 (c) : int32x2ComplexHalfAngleQ23 (c);
}

//SLICE
bool int32x2AngleLe (Int32x2 a, Int32x2 b) {
	const Int32
		la = int32x2ComplexLogj2Bit (a),
		lb = int32x2ComplexLogj2Bit (b);
	return
		la!=lb ? la<lb : int32x2MulVector (b,a) < 0;
}

//SLICE
void int32x2Cordic32TableCalculate_en (int e, Int32x2 *table) {
	const Int32x2 v = { -1<<e, 0 };
	table[0] = v;
	for (int i=1; i<32; i++) table[i] = int32x2ScaleTo1_en (e, int32x2ComplexHalfAngle (table[i-1]));
}

//SLICE
const Int32x2 cordicTable_e30[32] = {
	{	0xc0000000,	0x00000000	},
	{	0x00000000,	0x40000000	},
	{	0x2d413ccd,	0x2d413ccd	},
	{	0x3b20d79e,	0x187de2a7	},
	{	0x3ec52fa0,	0x0c7c5c1e	},
	{	0x3fb11b48,	0x0645e9af	},
	{	0x3fec43c7,	0x0323ecbe	},
	{	0x3ffb10c1,	0x0192155f	},
	{	0x3ffec42d,	0x00c90e90	},
	{	0x3fffb10b,	0x006487c4	},
	{	0x3fffec43,	0x003243f1	},
	{	0x3ffffb11,	0x001921fb	},
	{	0x3ffffec4,	0x000c90fe	},
	{	0x3fffffb1,	0x0006487f	},
	{	0x3fffffec,	0x0003243f	},
	{	0x3ffffffb,	0x00019220	},
	{	0x3fffffff,	0x0000c910	},
	{	0x40000000,	0x00006488	},
	{	0x40000000,	0x00003244	},
	{	0x40000000,	0x00001922	},
	{	0x40000000,	0x00000c91	},
	{	0x40000000,	0x00000648	},
	{	0x40000000,	0x00000324	},
	{	0x40000000,	0x00000192	},
	{	0x40000000,	0x000000c9	},
	{	0x40000000,	0x00000065	},
	{	0x40000000,	0x00000032	},
	{	0x40000000,	0x00000019	},
	{	0x40000000,	0x0000000d	},
	{	0x40000000,	0x00000006	},
	{	0x40000000,	0x00000003	},
	{	0x40000000,	0x00000001	},	// hand-edited this one...
};

//SLICE
Int32x2 int32x2RotTable_en (int e, const Int32x2 *table_en, Int32 angle_e32) {
	Int32x2 v = { 1<<e, 0 };
	for (int i=0; i<32; i++) {
		if (angle_e32 & 1<<31-i) v = int32x2MulComplex_en (e,v,table_en[i]);
	}
	return v;
}

//SLICE
Int32x2 int32x2Rot_en (int e, Int32 angle_e32) {
	return int32x2MulExp2 (
		int32x2RotTable_en (30,cordicTable_e30,angle_e32),
		e-30
	);
}

//SLICE
Int32 int32x2AngleTable_e32 (int e, const Int32x2 *table_en, Int32x2 x) {
	Int32x2 v = { 1<<e, 0 };
	Int32 phi_e32 = 0;
	for (int i=0; i<32; i++) {
		const Int32x2 vMore = int32x2MulComplex_en (e,v,table_en[i]);
		if (int32x2AngleLe (vMore,x)) {
			phi_e32 |= 1<<31-i;
			v = vMore;
		}
	}
	return phi_e32;
}

//SLICE
Int32 int32x2Angle_e32 (Int32x2 x) {
	return int32x2AngleTable_e32 (30,cordicTable_e30,x);
}

//SLICE
Int32x2 int32x2PolarToCartesian_een (int eP, int eAngle, Int32x2 p) {
	const Int32 angle_e32 = int32MulExp2 (p.y, 32-eAngle);

	return int32x2Scale_en (eP,
		int32x2Rot_en (eP,angle_e32),
		p.x
	);
}

//SLICE
// tested
Int32x2 int32x2CartesianToPolar_een (int e, int eAngle, Int32x2 x) {
	const Int32 s_e32 = int32x2Angle_e32 (x);
	Int32x2 p = { int32x2Abs (x), int32MulExp2 (s_e32,eAngle-32) };
	return p;
}

//SLICE
// tested
Int32x2 int32x2CartesianToPolarFrom_een (int eP, int eAngle, Int32x2 p0, Int32x2 x) {
	const Int32 s_e32 = int32x2Angle_e32 (x);
	const Int32 s0_e32 = int32MulExp2 (p0.y,32-eAngle);
	const Int32 diff_eAngle = int32MulExp2 (s_e32-s0_e32, eAngle-32);
	Int32x2 p = { int32x2Abs (x), p0.y+diff_eAngle };
	return p;
}


//SLICE
/*
% a dirA  b dirB complexLineSegmentIntersect { s true | false }
/complexIntersectLineLine { 6 dict begin
	[/dirB /b /dirA /a ] { exch def } forall

	/p dirB dirA complexMulSin def
	p abs 1E-10 ge {
		/lb a b vectorSub  dirA complexMulSin p div def
		b dirB lb vectorScale vectorAdd
		true
	}
	{
		false
	} ifelse
end } def
*/
bool int32x2IntersectLineLine_en (int e, const Int32x2 a, Int32x2 dirA, Int32x2 b, Int32x2 dirB, Int32x2 *point) {
	Int32 p_en = int32x2MulVector_en (e,dirB,dirA);
	if (int32Abs(p_en) > 0) {	// minimum requirement
	//if (int32Abs (int32x2MulVector_en (e, int32x2ScaleTo1_en(e,dirA), int32x2ScaleTo1_en(e,dirB)))>= 1<<e/2) {	// precision required
		const Int32 lb_en = int32Div_en (e,int32x2MulVector_en (e,int32x2Sub (a,b),dirA),p_en);
		if (point!=0) *point = int32x2Add (b,int32x2Scale_en (e,dirB,lb_en));
		return true;
	}
	else return false;
}

//SLICE
bool int32x2IntersectLineSegment_en (int e, const Int32x2 g, Int32x2 dirG, Int32x2 a, Int32x2 b, Int32x2 *point) {
	Int32x2 p;
	if (int32x2IntersectLineLine_en (e,g,dirG,a, int32x2Sub (b,a), &p)
	&& int32x2IsScalarBetween (a,b,p)) {
		if (point!=0) *point = p;
		return true;
	}
	else return false;
}

//SLICE
Int32x2 int32x2IntersectLineLineBetween_en (int e, Int32 sineLimit_en, const Int32x2 a, Int32x2 dirA, Int32x2 b, Int32x2 dirB) {
	Int32 p_en = int32x2MulVector_en (e,dirB,dirA);
	Int32 sineScaled_en = ((Int64)int32x2Abs(dirA)*int32x2Abs(dirB) >>e) * sineLimit_en >>e;
	if (int32Abs(p_en) > sineScaled_en ) {	// minimum requirement
		const Int32 lb_en = int32Div_en (e,int32x2MulVector_en (e,int32x2Sub (a,b),dirA),p_en);
		return int32x2Add (b,int32x2Scale_en (e,dirB,lb_en));
	}
	else return int32x2Avg (a,b);
}

//SLICE
bool int32x2IsCriticalIntersectLineLine_een (int e, Int32 sineLimit_en, const Int32x2 dirA, const Int32x2 dirB) {
	Int32 p_en = int32x2MulVector_en (e,dirB,dirA);
	Int32 sineScaled_en = ((Int64)int32x2Abs(dirA)*int32x2Abs(dirB) >>e) * sineLimit_en >>e;
	return !(int32Abs(p_en) > sineScaled_en );
}

//SLICE
bool int32x2IsAngularBetween (const Int32x2 a, const Int32x2 x, const Int32x2 b) {
	if (int32x2MulVector (a,b) >=0) return int32x2MulVector (a,x) >=0 && int32x2MulVector (x,b) >=0;
	else return int32x2MulVector (a,x) <=0 && int32x2MulVector (x,b) <=0;
}

//SLICE
bool fifoPrintInt32x2Complex_en (Fifo *fifo, int e, int fractional, Int32x2 v) {
	return	fifoPrintInt32_en (fifo,e,v.x,1,fractional)
		&& ( v.y>=0 ?  fifoPrintString (fifo,"+j") && fifoPrintUint32_en (fifo,e,v.y,1,fractional)
		: fifoPrintString (fifo,"-j") && fifoPrintUint32_en (fifo,e,-(Uint32)v.y,1,fractional)
		)
		;
}

//SLICE
bool fifoPrintInt32x2Complex_een (Fifo *fifo, int ex, int ey, int fractional, Int32x2 v) {
	return	fifoPrintInt32_en (fifo,ex,v.x,1,fractional)
		&& ( v.y>=0 ?  fifoPrintString (fifo,"+j") && fifoPrintUint32_en (fifo,ey,v.y,1,fractional)
		: fifoPrintString (fifo,"-j") && fifoPrintUint32_en (fifo,ey,-(Uint32)v.y,1,fractional)
		)
		;
}

//SLICE
bool fifoPrintInt32x2HexComplex (Fifo *fifo, Int32x2 v) {
	return	fifoPrintString (fifo,"0x")
		&& fifoPrintHex (fifo,v.x,8,8)
		&& fifoPrintString (fifo,"+j0x")
		&& fifoPrintHex (fifo,v.y,8,8)
		;
}

//SLICE
bool fifoPsPrintComplex_en (Fifo *fifo, int e, Int32x2 value) {
	return	fifoPrintChar (fifo,'[')
		&& fifoPrintInt32_en (fifo, e, value.x, 2, 3+e/4)
		&& fifoPrintChar (fifo,' ')
		&& fifoPrintInt32_en (fifo, e, value.y, 2, 3+e/4)
		&& fifoPrintChar (fifo,']');
}

//SLICE
bool fifoPsPrintString (Fifo *fifo, const char* s) {
	bool success = fifoPrintChar (fifo,'(');
	while (*s!=0 && success) {
		switch(*s) {
			case '(':
			case ')':	success = success && fifoPrintChar (fifo,'\\');	// no break here, intentionally!
			default:	success = success && fifoPrintChar (fifo,*s);
		}
		s++;
	}
	return success && fifoPrintChar (fifo,')');
}

//SLICE
bool fifoPsDrawComplexBasic_en (Fifo *fifo, int e, Int32x2 c1, Int32x2 c2, const char* label, const char *function) {
	return	fifoPsPrintComplex_en (fifo,e,c1)
		&& fifoPrintChar (fifo,' ')
		&& fifoPsPrintComplex_en (fifo,e,c2)
		&& fifoPrintChar (fifo,' ')
		&& (label==0?true : fifoPsPrintString (fifo,label) && fifoPrintChar (fifo,' '))
		&& fifoPrintStringLn (fifo, function)
		;
}

//SLICE
bool fifoPsDrawComplex_en (Fifo *fifo, int e, Int32x2 where, Int32x2 value, const char* label) {
	return fifoPsDrawComplexBasic_en (fifo,e,where,value,label,"drawComplex");
}

//SLICE
bool fifoPsDrawComplexFromTo_en (Fifo *fifo, int e, Int32x2 from, Int32x2 to, const char* label) {
	return fifoPsDrawComplexBasic_en (fifo,e,from,to,label,"drawComplexFromTo");
}

//SLICE
bool fifoPsDrawComplexSegment_en (Fifo *fifo, int e, Int32x2 where, Int32x2 value, const char* label) {
	return fifoPsDrawComplexBasic_en (fifo,e,where,value,label,"drawComplexSegment");
}

//SLICE
bool fifoPsDrawComplexSegmentFromTo_en (Fifo *fifo, int e, Int32x2 from, Int32x2 to, const char* label) {
	return fifoPsDrawComplexBasic_en (fifo,e,from,to,label,"drawComplexSegmentFromTo");
}

//SLICE
bool fifoPsDrawComplexLine_en (Fifo *fifo, int e, Int32x2 point, Int32x2 direction) {
	return fifoPsDrawComplexBasic_en (fifo,e,point,direction,0,"drawComplexLine");
}

//SLICE
bool fifoPsDrawBezier2_en (Fifo *fifo, int e, Int32x2 x0, Int32x2 x1, Int32x2 x2) {
	return	fifoPsPrintComplex_en (fifo,e,x0)
		&& fifoPrintChar (fifo,' ')
		&& fifoPsPrintComplex_en (fifo,e,x1)
		&& fifoPrintChar (fifo,' ')
		&& fifoPsPrintComplex_en (fifo,e,x2)
		&& fifoPrintStringLn (fifo, " drawBezier2Segment")
		;
}

//SLICE
bool fifoPsDrawBezier3_en (Fifo *fifo, int e, Int32x2 x0, Int32x2 x1, Int32x2 x2, Int32x2 x3) {
	return	fifoPsPrintComplex_en (fifo,e,x0)
		&& fifoPrintChar (fifo,' ')
		&& fifoPsPrintComplex_en (fifo,e,x1)
		&& fifoPrintChar (fifo,' ')
		&& fifoPsPrintComplex_en (fifo,e,x2)
		&& fifoPrintChar (fifo,' ')
		&& fifoPsPrintComplex_en (fifo,e,x3)
		&& fifoPrintStringLn (fifo, " drawBezier3Segment")
		;
}

//SLICE
bool fifoPsDrawPolygon_en (Fifo *fifo, int e, const Int32x2 *xs, int n) {
	bool success = fifoPrintChar (fifo, '[');
	for (int i=0; success && i<n; i++) {
		success = success
			&& fifoPsPrintComplex_en (fifo,e,xs[i])
			&& fifoPrintChar (fifo,' ')
			;
	}
	return success && fifoPrintStringLn (fifo,"] drawPolygon");
}

//SLICE
bool fifoPsDrawComplexMark_en (Fifo *fifo, int e, const Int32x2 p, const char *label) {
	return fifoPsPrintComplex_en (fifo,e,p) && fifoPsPrintString (fifo,label) && fifoPrintString (fifo, " drawComplexMark");
}

//SLICE
bool fifoPsDrawComplexCircle_en (Fifo *fifo, int e, const Int32x2 center, Int32 r) {
	return	fifoPsPrintComplex_en (fifo,e,center)
		&& fifoPrintUint32_en (fifo,e,r,2,3+e/4)
		&& fifoPrintStringLn (fifo, " drawComplexCircle");
}

