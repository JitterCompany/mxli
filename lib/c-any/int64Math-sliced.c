//HEADER
/*
  int64Math-sliced.c 
  Copyright 2012-2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <int64Math.h>

//SLICE
Uint64 uint64Div (Uint64 a, Uint64 b) {
	if (b==0) return 0;

	int lz = 0;
	while (((b<<lz) & (1llu<<63))==0) lz++;

	Uint64 q = 0;
	for (int i=lz; i>=0; --i) {
		if (a >= (b<<i)) {
			a -= b<<i;
			q |= 1llu<<i;	// omission of llu size specifier was a really bad error for a long time !!
		}
	}
	return q;
}

//SLICE
#ifdef NATIVE_64BIT
// nothing
#else
Uint64 uint64Mod(Uint64 a, Uint64 b) {
	return a - b*uint64Div(a,b);
}
#endif

//SLICE
#ifdef NATIVE_64BIT
// nothing
#else
Int64 int64Div(Int64 a, Int64 b) {
	int sign = 1;
	if (a<0) {
		sign *= -1;
		a = -a;
	}
	if (b<0) {
		sign *= -1;
		b = -b;
	}
	return ((Int64)uint64Div(a,b))*sign;
}
#endif

//SLICE
Int64 int64Mod(Int64 a, Int64 b) {
	return a - b*int64Div(a,b);
}

//SLICE
Uint64 uint64DivFast_en (int e, Uint64 a, Uint64 b) {
	if (b==0 || a==0) return 0;

	int lzb = 0;
	while ((b&1llu<<63)==0) lzb++, b<<=1;

	int lza = 0;
	while ((a&1llu<<63)==0) lza++, a<<=1;

	Uint64 q = 0;
	for (int i=31; i>=0; --i, b>>=1) {
		if (a >= b) {
			a -= b;
			q |= 1<<i;
		}
	}
	const int shiftL = -31 +e -lza +lzb;
	return shiftL>0 ? q << shiftL : q >> -shiftL;
}

//SLICE
Uint64 uint64MulFast_en (int e, Uint64 a, Uint64 b) {
	if (b==0 || a==0) return 0;

	int lzb = 0;
	while ((b&1llu<<63-lzb)==0) lzb++;

	int lza = 0;
	while ((a&1llu<<63-lza)==0) lza++;

	const int lz = lza+lzb;

	// MSB overflow unless we have lza+lzb >= 64

	// lossless shift
	while (lza+lzb < 64 && (a&b&1)==0) {	// trailing zeroes available
		if ((a&1)==0) a>>=1, lza++;
		else b>>=1, lzb++;
	}

	// lossy shift if all else fails
	while (lza+lzb < 64) {
		if (lza>lzb) a>>=1, lza++;
		else b>>=1, lzb++;
	}

	const int shiftR = e -lza -lzb + lz;	// e - lossless shifts - lossy shifts
	return shiftR>=0 ? a*b >> shiftR : a*b << -shiftR;
}


//SLICE
Uint32 uint32SqrtFloor(Uint32 x) {
	if (x<=0) return x;	// catches x=0, too

	Uint32 r = 1, rr = 0;
	while (r!=rr) {
		rr = r;
		r = (r + uint64Div(x,r) + 1) >> 1;	
	}
	return r*r<=x && r!=0 ? r : r-1;
}

//SLICE
Uint32 uint32SqrtCeil(Uint32 x) {
	if (x<=0) return x;	// catches x=0, too

	Uint32 r = 1, rr = 0;
	while (r!=rr) {
		rr = r;
		r = (r + uint64Div(x,r) + 1) >> 1;	
	}
	return r*r>=x ? r : r+1;
}

//SLICE
/* Greedy/Bisection algorithm.
 */
Uint32 uint64SqrtFloor (Uint64 r) {
	Uint32 root = 0;

	for (int i=31; i>=0; i--) {
		if ((Uint64)(root+(1u<<i))*(root+(1u<<i)) <= r) root |= 1<<i;
	}

	return root;
}

//SLICE
Uint32 uint32SqrtFloor_en(int e, Uint32 _x) {
	Uint64 x = _x;
	int p = 0;
	while ((x & 1llu<<63)==0 && p<e) p++, x<<=1;	// do the best we can

	if (p-e & 1) p--, x>>=1;	// avoid sqrt(2) factor

	return p-e >=0 ? uint64SqrtFloor(x) << (p-e)/2 : uint64SqrtFloor(x) >> (e-p)/2;
}

// This function uses 64 bits only (not 128bits) to trade
Uint64 uint64SqrtFloorFast_en (int e, Uint64 x) {
	int p = 0;
	while ((x & 1llu<<63)==0 && p<e) p++, x<<=1;	// do the best we can

	if (p-e & 1) p--, x>>=1;	// avoid sqrt(2) factor

	return p-e >=0 ? uint64SqrtFloor(x) << (p-e)/2 : uint64SqrtFloor(x) >> (e-p)/2;
}


//SLICE
Int32 int32LinearExtrapolate(Int32 x0, Int32 y0, Int32 x1, Int32 y1, Int32 x) {
	return int64Div( int64RoundForDivide((Int64)y0*(x1-x) + (Int64)y1*(x-x0),(x1-x0)), (x1-x0) );
}

//SLICE
Int32 int32LinearInterpolate(Int32 x0, Int32 y0, Int32 x1, Int32 y1, Int32 x) {
	if (x<x0) return y0;
	else if (x>x1) return y1;
	else return int32LinearExtrapolate(x0,y0,x1,y1,x);
}

