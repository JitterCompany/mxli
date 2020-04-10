//HEADER
/*
  printFixedPoint-sliced.c 
  Copyright 2014 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <printFixedPoint.h>
#include <print.h>
#include <int32Math.h>
#include <uint16Div.h>
#include <uint32Div.h>

//SLICE
void printUint32(Uint32 value, int minWidth) {
	char digitBuffer[10];
	int digits = 0;
	do {
		digitBuffer[digits] = '0' + uint32Mod10(value);
		value = uint32Div10(value);
		digits++;
	} while((value!=0) && digits<sizeof digitBuffer);

	while (digits<minWidth) { printChar(' '); minWidth--; }	// padding
	while (digits) printChar(digitBuffer[--digits]);
}

//SLICE
void printUint16(Uint32 value, int minWidth) {
	char digitBuffer[5];
	int digits = 0;
	do {
		digitBuffer[digits] = '0' + uint16Mod10(value);
		value = uint16Div10(value);
		digits++;
	} while((value!=0) && digits<sizeof digitBuffer);

	while (digits<minWidth) { printChar(' '); minWidth--; }	// padding
	while (digits) printChar(digitBuffer[--digits]);
}

//SLICE
void printUint16Prefix(Uint32 value, char prefix, int minWidth) {
	char digitBuffer[5];
	int digits = 0;
	do {
		digitBuffer[digits] = '0' + uint16Mod10(value);
		value = uint16Div10(value);
		digits++;
	} while((value!=0) && digits<sizeof digitBuffer);

	if (prefix) minWidth--;
	while (digits<minWidth) { printChar(' '); minWidth--; }	// padding
	if (prefix) printChar(prefix);
	if (minWidth>0 || digits>1 || digitBuffer[0]!='0') while (digits) printChar(digitBuffer[--digits]);
}

//SLICE
void printUint32Prefix(Uint32 value, char prefix, int minWidth) {
	char digitBuffer[10];
	int digits = 0;
	do {
		digitBuffer[digits] = '0' + uint32Mod10(value);
		value = uint32Div10(value);
		digits++;
	} while((value!=0) && digits<sizeof digitBuffer);

	if (prefix) minWidth--;
	while (digits<minWidth) { printChar(' '); minWidth--; }	// padding
	if (prefix) printChar(prefix);
	if (minWidth>0 || digits>1 || digitBuffer[0]!='0') while (digits) printChar(digitBuffer[--digits]);
}

//SLICE
void printInt32(Int32 value, int minWidth) {
	if (value>=0) printUint32(value,minWidth);
	else printUint32(-(Uint32)value, minWidth ? -minWidth : -1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// E10 functions
//
//SLICE
void printUint32_e10(Uint32 value, int minWidth, int precision) {
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

	printUint32(integral,minWidth);

	// fractional part
	if (precision>=1) {
		printChar('.');
		precision--;
	}
	while (precision-- >0) {
		fractional *= 10;
		printChar('0' + (fractional>>point));
		fractional &= E-1;
	}
}

//SLICE
void printUint32Prefix_e10(Uint32 value, char prefix, int minWidth, int precision) {
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

	printUint32Prefix(integral,prefix,minWidth);

	// fractional part
	if (precision>=1) {
		printChar('.');
		precision--;
	}
	while (precision-- >0) {
		fractional *= 10;
		printChar('0' + (fractional>>point));
		fractional &= E-1;
	}
}

//SLICE
void printInt32_e10(Int32 value, int minWidth, int precision) {
	if (value>=0) printUint32Prefix_e10((Uint32)value,minWidth<0?'+':0, int32Abs(minWidth),precision);
	else printUint32Prefix_e10((Uint32) -value,'-',int32Max(1, int32Abs(minWidth)), precision);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// E16 functions
//
//SLICE
void printUint32_e16(Uint32 value, int minWidth, int precision) {
	enum {
		point = 16,
		E = 1<<point,
	};

	//  do rounding: +0.5*lowest digit.
	Uint32 round = E/2;	// less than 1<<16 !
	for (int i=1; i<precision; i++) round = uint16Div10(round);
	value += round;

	Uint32 integral = value >> point;
	Uint32 fractional = value & E-1;

	printUint16(integral,minWidth);

	// fractional part
	if (precision>=1) {
		printChar('.');
		precision--;
	}
	while (precision-- >0) {
		fractional *= 10;
		printChar('0' + (fractional>>point));
		fractional &= E-1;
	}
}

//SLICE
void printUint32Prefix_e16(Uint32 value, char prefix, int minWidth, int precision) {
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

	printUint16Prefix(integral,prefix,minWidth);

	// fractional part
	if (precision>=1) {
		printChar('.');
		precision--;
	}
	while (precision-- >0) {
		fractional *= 10;
		printChar('0' + (fractional>>point));
		fractional &= E-1;
	}
}

//SLICE
void printInt32_e16(Int32 value, int minWidth, int precision) {
	if (value>=0) printUint32Prefix_e16((Uint32)value,minWidth<0?'+':0, int32Abs(minWidth),precision);
	else printUint32Prefix_e16((Uint32) -value,'-',int32Max(1, int32Abs(minWidth)), precision);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// E20 functions
//
//SLICE
void printUint32_e20(Uint32 value, int minWidth, int precision) {
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

	printUint16(integral,minWidth);

	// fractional part
	if (precision>=1) {
		printChar('.');
		precision--;
	}
	while (precision-- >0) {
		fractional *= 10;
		printChar('0' + (fractional>>point));
		fractional &= E-1;
	}
}

//SLICE
void printUint32Prefix_e20(Uint32 value, char prefix, int minWidth, int precision) {
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

	printUint16Prefix(integral,prefix,minWidth);

	// fractional part
	if (precision>=1) {
		printChar('.');
		precision--;
	}
	while (precision-- >0) {
		fractional *= 10;
		printChar('0' + (fractional>>point));
		fractional &= E-1;
	}
}

//SLICE
void printInt32_e20(Int32 value, int minWidth, int precision) {
	if (value>=0) printUint32Prefix_e20((Uint32)value,minWidth<0?'+':0, int32Abs(minWidth),precision);
	else printUint32Prefix_e20((Uint32) -value,'-',int32Max(1, int32Abs(minWidth)), precision);
}

