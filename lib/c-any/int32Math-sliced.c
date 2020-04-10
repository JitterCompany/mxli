//HEADER
/*
  int32Math-sliced.c 
  Copyright 2012-2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <int32Math.h>

//SLICE
Int32 int32MathDiv(Int32 a, Int32 b) {
	if (b>=0)
		if (a>=0) return a/b;
		else return (a-b+1) / b;
	else return int32MathDiv(-a,-b);
}

//SLICE
Int32 int32MathMod(Int32 a, Int32 b) {
	if (b>=0) {
		const int r = a % b;
		if (r>=0) return r;
		else return r + b;
	}
	else return int32MathMod(-a,-b);
}

//SLICE
Uint32 uint32Power(Uint32 x, Uint32 y) {
	Uint32 p = 1;
	for (int i=0; i<y; ++i, p*=x);

	return p;
}

//SLICE
Int32 int32Quantize(Int32 step, Int32 value) {
	return step * int32MathDiv(value + (step>>1),step);
}

//SLICE
Int32 int32QuantizeExp2(Int32 logStep, Int32 value) {
	return (value + (1<<logStep-1)) >> logStep << logStep;
}

//SLICE
int int32BoundedSigned(int value, int bitMask) {
	const int maximum = bitMask>>1;
	const int minimum = -maximum-1;
	const int shift = bitMask+1;

	if (value>maximum) return value-shift;
	else if (value<minimum) return value+shift;
	else return value;
}

//SLICE
Uint32 uint32BitReverse(Uint32 value, Uint32 length) {
	Uint32 result = 0;
	for (int i=0; i<length; ++i) result |= value & 1<<i ? 1<<length-1-i : 0;

	return result;
}

//SLICE
Int32 int32LinearExtrapolateFast(Int32 x0, Int32 y0, Int32 x1, Int32 y1, Int32 x) {
	return int32RoundForDivide( y0*(x1-x) + y1*(x-x0),(x1-x0)) / (x1-x0);
}

//SLICE
bool int32CanLinearInterpolateFast(Int32 x0, Int32 y0, Int32 x1, Int32 y1) {
	Int64 large = ((Int64)(x1)+x0) * ((Int64)(y1)+y0);
	return (large < (1ll<<31)) || ( large >= (-1ll<<31));
}

//SLICE
Int32 int32LinearInterpolateFast(Int32 x0, Int32 y0, Int32 x1, Int32 y1, Int32 x) {
	if (x<x0) return y0;
	else if (x>x1) return y1;
	else return int32LinearExtrapolateFast(x0,y0,x1,y1,x);
}

//SLICE
Uint16 uint32SqrtFloor (Uint32 r) {
	Uint32 root = 0;

	for (int i=15; i>=0; i--) {
		if ((root+(1u<<i))*(root+(1u<<i)) <= r) root |= 1<<i;
	}

	return root;
}

//SLICE
//64-bit math is avoided at the cost of precision.
Uint32 uint32SqrtFloorFast_en (int e, Uint32 x) {

	int p = 0;
	while ((x & 1u<<31)==0 && p<e) p++, x<<=1;	// do the best we can

	if (e-p & 1) p--, x>>=1;	// avoid sqrt(2) factor

	return (e-p) >=0 ? (Uint32)uint32SqrtFloor(x) << (e-p)/2 : (Uint32)uint32SqrtFloor(x) >> (p-e)/2;
}

