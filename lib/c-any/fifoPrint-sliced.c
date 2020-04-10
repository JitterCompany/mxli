//HEADER
/*
  fifoPrint-sliced.c 
  Copyright 2012-2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <fifoPrint.h>
#include <string.h>
#include <macros.h>
#include <int32Math.h>
#include <int64Math.h>


//SLICE
bool fifoPrintChar(Fifo *fifo, char c) {
	if (fifoCanWrite(fifo)>0) {
		fifoWrite(fifo,c);
		return true;
	}
	else return false;
}

//SLICE
bool fifoPrintCharEscaped(Fifo *fifo, char c) {
	if (fifoCanWrite(fifo)>0) {
		if (c<32 || c>=127) return fifoPrintString(fifo,"\\x") && fifoPrintHex(fifo,c,2,2);
		else {
			fifoWrite(fifo,c);
			return true;
		}
	}
	else return false;
}
//SLICE
bool fifoPrintString(Fifo *fifo, const char *string) {
	if (string) {
		for ( ; *string ; ++string)
			if (fifoCanWrite(fifo)>0) fifoWrite(fifo,*string);
			else return false;
		return true;
	}
	else {
		fifoPrintString (fifo,"(const char*)<NULL>");
		return false;
	}
}

//SLICE
bool fifoPrintLn(Fifo *fifo) {
	return fifoPrintChar(fifo,'\r') && fifoPrintChar(fifo,'\n');
}

//SLICE
bool fifoPrintChars(Fifo *fifo, const char *chars, unsigned n) {
	for (int i=0 ; i<n; ++i)
		if (fifoCanWrite(fifo)) fifoWrite(fifo,chars[i]);
		else return false;
	return true;
}

//SLICE
bool fifoPrintBaseNChar (Fifo *fifo, int value) {
	if (fifoCanWrite(fifo)>0) {
		fifoWrite(fifo, value<10 ? value+'0' : value+'A'-0xA);
		return true;
	}
	else return false;
}

//SLICE
bool fifoPrintHex(Fifo *fifo, unsigned value, int minWidth, int maxWidth) {
	int width = 0;

	for (unsigned v=value; v!=0; v>>=4, width++) ;
	const int chosenWidth = MAX(minWidth, MIN(width,maxWidth));

	for (int digit=chosenWidth-1; digit>=0; digit--) {
		int digitValue = value>>(4*digit) & 0xF;
		if (fifoCanWrite(fifo)>0) fifoWrite(fifo, digitValue<10 ? digitValue+'0' : digitValue+'A'-0xA);
		else return false;
	}
	return true;
}

//SLICE
bool fifoPrintBin(Fifo *fifo, unsigned value, int minWidth, int maxWidth) {
	int width = 0;

	for (unsigned v=value; v!=0; v>>=1, width++) ;
	const int chosenWidth = MAX(minWidth, MIN(width,maxWidth));

	for (int digit=chosenWidth-1; digit>=0; digit--) {
		int digitValue = value>>digit & 1;
		if (fifoCanWrite(fifo)>0) fifoWrite(fifo, digitValue+'0');
		else return false;
	}
	return true;
}

//SLICE
bool fifoPrintBinFixed(Fifo *fifo, unsigned value, int width, int groupSize) {

	for (int digit=width-1; digit>=0; digit--) {
		int digitValue = value>>digit & 1;
		if (fifoCanWrite(fifo)>0) fifoWrite(fifo, digitValue+'0');
		else return false;

		if (digit%groupSize ==0
		&& digit!=0) {
			if (fifoCanWrite(fifo)>0) fifoWrite(fifo,',');
			else return false;
		}
	}
	return true;
}

//SLICE
bool fifoPrintHex32Masked(Fifo *fifo, unsigned value, unsigned mask, char dontCare) {
	for (int i=0; i<8; i++) {
		const unsigned digit = value>>28;
		const unsigned m = mask>>28;
		if (fifoCanWrite(fifo)>0) {
			if (m==0) fifoWrite(fifo,dontCare);
			else if (m==0xF) fifoWrite(fifo, digit<10 ? digit+'0' : digit+'A'-0xA);
			else {
				fifoWrite(fifo,'!');
				return false;
			}
		}
		else return false;
	}
	return true;
}

bool fifoPrintHexString(Fifo *fifo, const char *data, int dataSize, int width) {
	bool success = true;
	for (int b=0; success && b<dataSize; b++) {
		if (b!=0 && b % width ==0) success = success && fifoPrintChar(fifo,' ');
		const int index = b/width*width + (width-1 - b % width);
		success = success && fifoPrintHex(fifo,index<dataSize ? data[index] : 0, 2,2);
	}
	return success;
}

//SLICE
bool fifoPrintBaseN(Fifo *fifo, unsigned value, unsigned minWidth, unsigned maxWidth, int base) {
	int width=1;
	unsigned power=1;

	for ( ; value/power>=base; power*=base, width++) ;

	const int chosenWidth = MAX(minWidth, MIN(width,maxWidth));
	int i;
	for (i=1,power=1; i<chosenWidth; ++i) power*=base;

	for (i=0; i<chosenWidth; ++i, value %=power, power/=base) {
		unsigned digitValue = value / power;
		if (fifoCanWrite(fifo)>0) fifoWrite(fifo, digitValue<10 ? '0'+digitValue : 'A'-0xA+digitValue);
		else return false;
	}
	return true;
}

//SLICE
bool fifoPrintBaseN64(Fifo *fifo, Uint64 value, unsigned minWidth, unsigned maxWidth, int base) {
	int width=1;
	Uint64 power=1;

	for ( ; value/power>=base; power*=base, width++) ;

	const int chosenWidth = MAX(minWidth, MIN(width,maxWidth));
	int i;
	for (i=1,power=1; i<chosenWidth; ++i) power*=base;

	for (i=0; i<chosenWidth; ++i, value = uint64Mod(value,power), power = uint64Div(power,base)) {
		unsigned digitValue = uint64Div(value,power);
		if (fifoCanWrite(fifo)>0) fifoWrite(fifo, digitValue<10 ? '0'+digitValue : 'A'-0xA+digitValue);
		else return false;
	}
	return true;
}

//SLICE
bool fifoPrintUDec(Fifo *fifo, unsigned value, unsigned minWidth, unsigned maxWidth) {
	return fifoPrintBaseN(fifo, value, minWidth, maxWidth,10);
}

//SLICE
bool fifoPrintUDec64(Fifo *fifo, Uint64 value, unsigned minWidth, unsigned maxWidth) {
	return fifoPrintBaseN64(fifo, value, minWidth, maxWidth,10);
}

//SLICE
bool fifoPrintSDec(Fifo *fifo, int value, unsigned minWidth, unsigned maxWidth, bool showPositive) {
	if (value>=0)
		if (showPositive) return fifoPrintChar(fifo,'+') && fifoPrintUDec(fifo,value,minWidth-1,maxWidth-1);
		else return fifoPrintUDec(fifo,value,minWidth,maxWidth);

	else return fifoPrintChar(fifo,'-') && fifoPrintUDec(fifo,-value,minWidth-1,maxWidth-1);
}

//SLICE
bool fifoPrintInt(Fifo *fifo, int value) {
	return fifoPrintSDec (fifo,value,1,11,false);
}

//SLICE
bool fifoPrintSDec64(Fifo *fifo, Int64 value, unsigned minWidth, unsigned maxWidth, bool showPositive) {
	if (value>=0)
		if (showPositive) return fifoPrintChar(fifo,'+') && fifoPrintUDec64(fifo,value,minWidth-1,maxWidth-1);
		else return fifoPrintUDec64(fifo,value,minWidth,maxWidth);

	else return fifoPrintChar(fifo,'-') && fifoPrintUDec64(fifo,-value,minWidth-1,maxWidth-1);
}

//SLICE
bool fifoPrintBaseNFraction(Fifo *fifo, unsigned value, unsigned scale, unsigned minWidth, unsigned maxWidth,
	unsigned fractionWidth, int base) {

	int roundPlus = scale/2;
	for (int p=1; p<fractionWidth; ++p) roundPlus /=base;
	value += roundPlus;

	int integralWidth = 0;
	
	for (unsigned v=value/scale; v!=0; v/=base, integralWidth++) ;
	const int chosenWidth = MAX(minWidth, MIN(integralWidth,maxWidth));

	// integral part
	unsigned integral = value / scale;
	for (int index=0; index<chosenWidth; index++) {
		int digitValue = integral / uint32Power(base,chosenWidth-1-index);
		integral %= uint32Power(base,chosenWidth-1-index);

		if (fifoCanWrite(fifo)) fifoPrintChar(fifo,digitValue<10 ? '0'+digitValue : 'A'-0xA+digitValue);
		else return false;
	}

	// fractional part
	unsigned fractional = (value % scale) * base;
	if (fractionWidth>=1)
		if (fifoCanWrite(fifo)) fifoPrintChar(fifo,'.');
		else return false;
	for (int f=1; f<fractionWidth; ++f) {
		int digitValue = fractional / scale;
		fractional = (fractional % scale) * base;
		if (fifoCanWrite(fifo)) fifoPrintChar(fifo, digitValue<10 ? '0'+digitValue : 'A'-0xA+digitValue);
		else return false;
	}
	return true;
}

//SLICE
bool fifoPrintFixedPointInt(Fifo *fifo, int value, int scale, unsigned minWidth, unsigned maxWidth,
	unsigned fracDigits, bool showPositive) {

	if ((long long)value*scale < 0) {
		if (!fifoPrintChar(fifo,'-')) return false;
		minWidth = minWidth!=0 ? minWidth-1 : 0;
		maxWidth = maxWidth!=0 ? maxWidth-1 : 0;
	}
	else if (showPositive) {
		if (!fifoPrintChar(fifo,'+')) return false;
		minWidth = minWidth!=0 ? minWidth-1 : 0;
		maxWidth = maxWidth!=0 ? maxWidth-1 : 0;
	}

	return fifoPrintBaseNFraction(fifo,ABS(value),ABS(scale),minWidth,maxWidth,fracDigits,10);
}

//SLICE
const char siPrefixMultiplier3[9] = " kMGTPEZY";

//SLICE
const char siPrefixDivider3[9]    = " munpfazy";

//SLICE
bool fifoPrintSiPrefix (Fifo *fifo, int exponent) {
	if (exponent==0) return true;
	else if (exponent%3) return fifoPrintChar (fifo,'!');	// deci/deka, etc not supported
	else if (exponent<0 && exponent> -3*sizeof siPrefixDivider3) return fifoPrintChar (fifo,siPrefixDivider3[exponent/-3]);
	else if (exponent>0 && exponent<  3*sizeof siPrefixMultiplier3) return fifoPrintChar (fifo,siPrefixMultiplier3[exponent/3]);
	else return fifoPrintChar (fifo,'?');
}

//SLICE
// Requires float calculations:
bool fifoPrintTechnicalFloat(Fifo *fifo, float value, int significandWidth) {
	if (significandWidth<3) return false;

	if (value>=0) if (!fifoPrintChar(fifo,'+')) return false;
	else {
		if (!fifoPrintChar(fifo,'-')) return false;
		value = -value;
	}

	int exponent3 = 0;
	if (value<1.f) while (value<1.f && exponent3>-8) {
		value *= 1000.f;
		exponent3--;
	}
	else while(value>=1000.f && exponent3<8) {
		value /= 1000.f;
		exponent3++;
	}

	static const float pow10[] = { 1.f, 10.f, 100.f };
	bool leadingZeros = true;
	for (int p=2; p>=0; --p) if (value>=pow10[p] || !leadingZeros) {
		leadingZeros = false;
		const int i = MIN(9,(int)(value/pow10[p]));		// gcc truncates here. This is desired behaviour.
		if (!fifoPrintChar(fifo,'0'+i)) return false;
		value -= pow10[p]*i;
		significandWidth--;
	}
	if (significandWidth>0) {
		if (!fifoPrintChar(fifo,'.')) return false;
		significandWidth--;
	}
	while (significandWidth>0) {
		value *= 10.f;
		const int i = MIN(9,(int)(value));			// gcc truncates here. This is desired behaviour.
		if (!fifoPrintChar(fifo,'0'+i)) return false;
		value -= i;
		significandWidth--;
	}

	if (exponent3>=1)
		if (!fifoPrintChar(fifo,siPrefixMultiplier3[exponent3])) return false;
		else;
	else if (exponent3<=-1)
		if (!fifoPrintChar(fifo,siPrefixDivider3[-exponent3])) return false;
		else;

	return true;
}

//SLICE
bool fifoPrintBin64(Fifo *fifo, unsigned long long value, unsigned minWidth, unsigned maxWidth) {
	int width = 0;

	for (unsigned v=value; v!=0; v>>=1, width++) ;
	const int chosenWidth = MAX(minWidth, MIN(width,maxWidth));

	for (int digit=chosenWidth-1; digit>=0; digit--) {
		int digitValue = value>>digit & 0x1;
		if (fifoCanWrite(fifo)>0) fifoWrite(fifo, digitValue+'0');
		else return false;
	}
	return true;
}

//SLICE
bool fifoPrintHex64(Fifo *fifo, unsigned long long value, unsigned minWidth, unsigned maxWidth) {
	int width = 0;

	for (unsigned v=value; v!=0; v>>=4, width++) ;
	const int chosenWidth = MAX(minWidth, MIN(width,maxWidth));

	for (int digit=chosenWidth-1; digit>=0; digit--) {
		int digitValue = value>>(digit*4) & 0xF;
		if (fifoCanWrite(fifo)>0) fifoWrite(fifo, digitValue<10 ? '0'+digitValue : 'A'+digitValue-10);
		else return false;
	}
	return true;
}

//SLICE
bool fifoDumpFifoAscii(Fifo *output, Fifo *dump) {
	bool success = true;
	while (fifoCanRead(dump) && success) {
		const int c = 0xFF & fifoRead(dump);
		if (c<32 || c>=127) success = success && fifoPrintString(output,"\\x") && fifoPrintHex(output,c,2,2);
		else success = success && fifoPrintChar(output,c);
	}
	return success;		
}

//SLICE
bool fifoDumpFifoHex(Fifo *output, Fifo *fifo, int bytesPerLine, bool ascii) {
	bool success = true;

	int address = 0;
	while (fifoCanRead(fifo) && success) {
		success = success && fifoPrintHex(output,address,4,4) && fifoPrintString(output,"  ");
		Fifo clone = *fifo;
		for (int i=0; i<bytesPerLine; ++i) {
			if (i!=0) success = success && fifoPrintChar(output,' ');
			if (fifoCanRead(&clone)) {
				const int c = 0xFF & fifoRead(&clone);
				address ++;
				success = success && fifoPrintHex(output,c,2,2);
			}
			else fifoPrintString(output,"__");
		}

		if (ascii) {
			success = success && fifoPrintChar(output,'|');
			for (int i=0; i<bytesPerLine; ++i) {
				if (fifoCanRead(fifo)) {
					const int c = 0xFF & fifoRead(fifo);
					if (c<32 || c>=127) success = success && fifoPrintChar(output,' ');
					else success = success && fifoPrintChar(output,c);
				}
				else fifoPrintChar(output,' ');
			}
			success = success && fifoPrintChar(output,'|');
		}
		else fifoCopyReadPosition(fifo,&clone);

		if (fifoCanRead(fifo)) success = success && fifoPrintLn(output);
	}
	return success;		
}

