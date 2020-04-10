//HEADER
/*
  fifoPrintFixedPoint-sliced.c 
  Copyright 2014 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <fifoPrintFixedPoint.h>
#include <print.h>
#include <int32Math.h>
#include <uint16Div.h>
#include <uint32Div.h>


//SLICE
bool fifoPrintUint16Pad(Fifo *fifo, Uint32 value, int minWidth, char padChar) {
	bool success = true;
	char digitBuffer[5];
	int digits = 0;
	do {
		digitBuffer[digits] = '0' + uint16Mod10(value);
		value = uint16Div10(value);
		digits++;
	} while((value!=0) && digits<sizeof digitBuffer);

	for (; success && digits<minWidth; minWidth--) success = success && fifoPrintChar(fifo, padChar);	// padding
	while (success && digits) success = success && fifoPrintChar(fifo, digitBuffer[--digits]);
	return success;
}

//SLICE
bool fifoPrintUint16(Fifo *fifo, Uint32 value, int minWidth) {
	bool success = true;
	char digitBuffer[5];
	int digits = 0;
	do {
		digitBuffer[digits] = '0' + uint16Mod10(value);
		value = uint16Div10(value);
		digits++;
	} while((value!=0) && digits<sizeof digitBuffer);

	for (; success && digits<minWidth; minWidth--) success = success && fifoPrintChar(fifo, ' ');	// padding
	while (success && digits) success = success && fifoPrintChar(fifo, digitBuffer[--digits]);
	return success;
}

//SLICE
bool fifoPrintUint32Pad(Fifo *fifo, Uint32 value, int minWidth, char padChar) {
	bool success = true;
	char digitBuffer[10];
	int digits = 0;
	do {
		digitBuffer[digits] = '0' + uint32Mod10(value);
		value = uint32Div10(value);
		digits++;
	} while((value!=0) && digits<sizeof digitBuffer);

	for (; success && digits<minWidth; minWidth--) success = success && fifoPrintChar(fifo, padChar);	// padding
	while (success && digits) success = success && fifoPrintChar(fifo, digitBuffer[--digits]);
	return success;
}

//SLICE
bool fifoPrintUint32(Fifo *fifo, Uint32 value, int minWidth) {
	bool success = true;
	char digitBuffer[10];
	int digits = 0;
	do {
		digitBuffer[digits] = '0' + uint32Mod10(value);
		value = uint32Div10(value);
		digits++;
	} while((value!=0) && digits<sizeof digitBuffer);

	for (; success && digits<minWidth; minWidth--) success = success && fifoPrintChar(fifo, ' ');	// padding
	while (success && digits) success = success && fifoPrintChar(fifo, digitBuffer[--digits]);
	return success;
}

//SLICE
bool fifoPrintUint16Prefix(Fifo *fifo, Uint32 value, char prefix, int minWidth) {
	bool success = true;
	char digitBuffer[5];
	int digits = 0;
	do {
		digitBuffer[digits] = '0' + uint16Mod10(value);
		value = uint16Div10(value);
		digits++;
	} while((value!=0) && digits<sizeof digitBuffer);

	if (prefix) minWidth--;
	for (; digits<minWidth; minWidth--) success = success && fifoPrintChar(fifo, ' ');	// padding
	if (prefix) success = success && fifoPrintChar(fifo, prefix);
	if (minWidth>0 || digits>1 || digitBuffer[0]!='0')
		while (success && digits) success = success && fifoPrintChar(fifo, digitBuffer[--digits]);
	return success;
}

//SLICE
// BUG found, 30.05.2016: :o) a full Fifo lead to endless loops, because the success condition wasn't tested (FIXED now).
// This kind of bug will probably be found in other functions here, too!!!!!!
bool fifoPrintUint32Prefix(Fifo *fifo, Uint32 value, char prefix, int minWidth) {
	bool success = true;
	char digitBuffer[10];
	int digits = 0;
	do {
		digitBuffer[digits] = '0' + uint32Mod10(value);
		value = uint32Div10(value);
		digits++;
	} while((value!=0) && digits<sizeof digitBuffer);

	if (prefix) minWidth--;
	for (; success && digits<minWidth; minWidth--) success = success && fifoPrintChar(fifo, ' ');	// padding
	if (prefix) success = success && fifoPrintChar(fifo, prefix);
	if (minWidth>0 || digits>1 || digitBuffer[0]!='0')
		while (digits && success) success = success && fifoPrintChar(fifo, digitBuffer[--digits]);
	return success;
}

//SLICE
bool fifoPrintInt16(Fifo *fifo, Int32 value, int minWidth) {
	if (value>=0) return fifoPrintUint16Prefix(fifo, value, 0, minWidth);
	else return fifoPrintUint16Prefix(fifo, -value,'-', minWidth ? -minWidth : -1);
}

//SLICE
bool fifoPrintInt32(Fifo *fifo, Int32 value, int minWidth) {
	if (value>=0) return fifoPrintUint32Prefix(fifo, value, 0, minWidth);
	else return fifoPrintUint32Prefix(fifo,-(Uint32)value,'-', minWidth ? -minWidth : -1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// E10 functions
//
//SLICE
bool fifoPrintUint32_e10(Fifo *fifo, Uint32 value, int minWidth, int precision) {
	enum {
		point = 10,
		E = 1<<point,
	};

	//  do rounding: +0.5*lowest digit.
	if (precision<=1)	value += E/2/10;	// zero digits below .
	else if (precision==2)	value += E/2/100;	// one digit below .
	else if (precision==3)	value += E/2/1000;	// two digits below .
	else ;						// no rounding at all

	Uint32 integral = value >> point;
	Uint32 fractional = value & E-1;

	bool success = fifoPrintUint32(fifo, integral,minWidth);

	// fractional part
	if (precision>=1) {
		success = success && fifoPrintChar(fifo, '.');
		precision--;
	}
	while (precision-- >0) {
		fractional *= 10;
		success = success && fifoPrintChar(fifo, '0' + (fractional>>point));
		fractional &= E-1;
	}
	return success;
}

//SLICE
bool fifoPrintUint32Prefix_e10(Fifo *fifo, Uint32 value, char prefix, int minWidth, int precision) {
	enum {
		point = 10,
		E = 1<<point,
	};

	//  do rounding: +0.5*lowest digit.
	if (precision<=1)	value += E/2/10;	// zero digits below .
	else if (precision==2)	value += E/2/100;	// one digit below .
	else if (precision==3)	value += E/2/1000;	// two digits below .
	else ;						// no rounding at all

	Uint32 integral = value >> point;
	Uint32 fractional = value & E-1;

	bool success = fifoPrintUint32Prefix(fifo, integral,prefix,minWidth);

	// fractional part
	if (precision>=1) {
		success = success && fifoPrintChar(fifo, '.');
		precision--;
	}
	while (precision-- >0) {
		fractional *= 10;
		success = success && fifoPrintChar(fifo, '0' + (fractional>>point));
		fractional &= E-1;
	}
	return success;
}

//SLICE
bool fifoPrintInt32_e10(Fifo *fifo, Int32 value, int minWidth, int precision) {
	if (value>=0) return fifoPrintUint32Prefix_e10(fifo, (Uint32)value,minWidth<0?'+':0, int32Abs(minWidth),precision);
	else return fifoPrintUint32Prefix_e10(fifo, (Uint32) -value,'-',int32Max(1, int32Abs(minWidth)), precision);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// E16 functions
//
//SLICE
bool fifoPrintUint32_e16(Fifo *fifo, Uint32 value, int minWidth, int precision) {
	enum {
		point = 16,
		E = 1<<point,
	};

	//  do rounding: +0.5*lowest digit.
	Uint32 round = E/2;
	for (int i=1; i<precision; i++) round = uint16Div10(round);
	value += round;

	Uint32 integral = value >> point;
	Uint32 fractional = value & E-1;

	bool success = fifoPrintUint16(fifo, integral,minWidth);

	// fractional part
	if (precision>=1) {
		success = success && fifoPrintChar(fifo, '.');
		precision--;
	}
	while (precision-- >0) {
		fractional *= 10;
		success = success && fifoPrintChar(fifo, '0' + (fractional>>point));
		fractional &= E-1;
	}
	return success;
}

//SLICE
bool fifoPrintUint32Prefix_e16(Fifo *fifo, Uint32 value, char prefix, int minWidth, int precision) {
	enum {
		point = 16,
		E = 1<<point,
	};

	//  do rounding: +0.5*lowest digit.
	Uint32 round = E/2;
	for (int i=1; i<precision; i++) round = uint16Div10(round);
	value += round;

	Uint32 integral = value >> point;
	Uint32 fractional = value & E-1;

	bool success = fifoPrintUint16Prefix(fifo, integral,prefix,minWidth);

	// fractional part
	if (precision>=1) {
		success = success && fifoPrintChar(fifo, '.');
		precision--;
	}
	while (precision-- >0) {
		fractional *= 10;
		success = success && fifoPrintChar(fifo, '0' + (fractional>>point));
		fractional &= E-1;
	}
	return success;
}

//SLICE
bool fifoPrintInt32_e16(Fifo *fifo, Int32 value, int minWidth, int precision) {
	if (value>=0) return fifoPrintUint32Prefix_e16(fifo, (Uint32)value,minWidth<0?'+':0, int32Abs(minWidth),precision);
	else return fifoPrintUint32Prefix_e16(fifo, (Uint32) -value,'-',int32Max(1, int32Abs(minWidth)), precision);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// E20 functions
//
//SLICE
bool fifoPrintUint32_e20(Fifo *fifo, Uint32 value, int minWidth, int precision) {
	enum {
		point = 20,
		E = 1<<point,
	};

	//  do rounding: +0.5*lowest digit.
	Uint32 round = E/2;
	for (int i=1; i<precision; i++) round = uint32Div10(round);
	value += round;

	Uint32 integral = value >> point;
	Uint32 fractional = value & E-1;

	bool success = fifoPrintUint16(fifo, integral,minWidth);

	// fractional part
	if (precision>=1) {
		success = success && fifoPrintChar(fifo, '.');
		precision--;
	}
	while (precision-- >0) {
		fractional *= 10;
		success = success && fifoPrintChar(fifo, '0' + (fractional>>point));
		fractional &= E-1;
	}
	return success;
}

//SLICE
bool fifoPrintUint32Prefix_e20(Fifo *fifo, Uint32 value, char prefix, int minWidth, int precision) {
	enum {
		point = 20,
		E = 1<<point,
	};

	//  do rounding: +0.5*lowest digit.
	Uint32 round = E/2;
	for (int i=1; i<precision; i++) round = uint32Div10(round);
	value += round;

	Uint32 integral = value >> point;
	Uint32 fractional = value & E-1;

	bool success = fifoPrintUint32Prefix(fifo, integral,prefix,minWidth);

	// fractional part
	if (precision>=1) {
		success = success && fifoPrintChar(fifo, '.');
		precision--;
	}
	while (precision-- >0) {
		fractional *= 10;
		success = success && fifoPrintChar(fifo, '0' + (fractional>>point));
		fractional &= E-1;
	}
	return success;
}

//SLICE
bool fifoPrintInt32_e20(Fifo *fifo, Int32 value, int minWidth, int precision) {
	if (value>=0) return fifoPrintUint32Prefix_e20(fifo, (Uint32)value,minWidth<0?'+':0, int32Abs(minWidth),precision);
	else return fifoPrintUint32Prefix_e20(fifo, (Uint32) -value,'-',int32Max(1, int32Abs(minWidth)), precision);
}

//SLICE
bool fifoPrintInt32Easy_e20(Fifo *fifo, Int32 value, int precision) {
	if (value>=0) return fifoPrintUint32Prefix_e20(fifo, (Uint32)value,0,1,precision);
	else return fifoPrintUint32Prefix_e20(fifo, (Uint32) -value,'-',2, precision);
}

//SLICE
bool fifoPrintUint32Prefix_en (Fifo *fifo, int e, Uint32 value, char prefix, int minWidth, int precision) {
	const Uint32 one = 1<<e;

	//  do rounding: +0.5*lowest digit.
	Uint32 round = one/2;
	for (int i=1; i<precision; i++) round = uint32Div10 (round);
	value += round;

	Uint32 integral = value >> e;
	Uint32 fractional = value & (one-1);

	bool success = fifoPrintUint32Prefix (fifo, integral,prefix,minWidth);

	// fractional part
	if (precision>=1) {
		success = success && fifoPrintChar (fifo, '.');
		precision--;
	}
	while (precision-- >0) {
		fractional *= 10;
		success = success && fifoPrintChar (fifo, '0' + (fractional>>e));
		fractional &= one-1;
	}
	return success;
}

//SLICE
bool fifoPrintInt32_en (Fifo *fifo, int e, Int32 value, int minWidth, int precision) {
	if (value>=0) return fifoPrintUint32Prefix_en (fifo,e, value, 0, minWidth,precision);
	else return fifoPrintUint32Prefix_en(fifo,e, -(Uint32)value,'-', minWidth ? -minWidth : -1,precision);
}

