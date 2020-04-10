//HEADER
/*
  fifoParse-sliced.c 
  Copyright 2012-2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <fifoParse.h>
#include <string.h>
#include <int64Math.h>

//SLICE
bool fifoParseExactChar(Fifo *text, char c) {
	if (fifoCanRead(text))
		if (c==fifoLookAhead(text)) {
			fifoRead(text);;
			return true;
		}
		else return false;	// wrong character
	else return false;	// no characters at all
}

//SLICE
bool fifoParseZ(Fifo *text) {
	return fifoParseExactChar(text,0);
}

//SLICE
bool fifoParseStringZ(Fifo *text, Fifo *result) {
	Fifo clone = *text;
	for (int length=0; fifoCanRead(&clone); length++) {
		if (fifoLookAhead(&clone)==0) {
			*result = *text;
			result->wTotal = result->rTotal + length; //clone.rTotal-text->rTotal;
			fifoRead(&clone);	// remove \0, too
			fifoCopyReadPosition(text,&clone);
			return true;
		}
		else fifoRead(&clone);
	}
	return false;
}

//SLICE
static bool contains(const char *set, char c) {
	for (; *set; set++) if (*set==c) return true;

	return false;
}

bool fifoParseCharSet(Fifo *text, char *c, const char *set) {
	if (fifoCanRead(text)
	&& contains(set,fifoLookAhead(text))) {
			*c = fifoRead(text);
			return true;
	}
	else return false;
}

//SLICE
bool fifoParseBlanks(Fifo *fifo) {
	bool success = false;
	while(fifoCanRead(fifo)) {
		if (isBlank(fifoLookAhead(fifo))) {
			success = true;
			fifoRead(fifo);
		}
		else return success;
	}
	return success;
}

//SLICE
bool fifoParseSpaces(Fifo *fifo) {
	bool success = false;
	while(fifoCanRead(fifo)) {
		if (isSpace(fifoLookAhead(fifo))) {
			success = true;
			fifoRead(fifo);
		}
		else return success;
	}
	return success;
}

//SLICE
bool fifoParseDigit(Fifo *fifo, int *value) {
	if (fifoCanRead(fifo) && isDigit(fifoLookAhead(fifo))) {
		*value = fifoRead(fifo) - '0';
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseHexDigit(Fifo *fifo, int *value) {
	if (fifoCanRead(fifo) && isHexDigit(fifoLookAhead(fifo))) {
		*value = baseNDigitToInt(fifoRead(fifo));
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseLineEnd(Fifo *fifo) {
	if (fifoCanRead(fifo)) {
		if (fifoLookAhead(fifo)=='\n') {
			fifoRead(fifo);
			return true;
		}
		else if (fifoLookAhead(fifo)=='\r') {
			fifoRead(fifo);
			if (fifoCanRead(fifo))
				if (fifoLookAhead(fifo)=='\n') fifoRead(fifo);	// consume \n after \r
				else ;	// non \n
			else ;	// end of stream
			return true;
		}
		else return false;	// other character
	}
	else return false;
}

//SLICE
bool fifoParseLineAuto(Fifo *fifo, FifoParseLineAuto *state, Fifo *fifoLine) {
	
	Fifo clone = *fifo;
	if (fifoCanRead(&clone) && fifoLookAhead(&clone)=='\n' && *state=='\r') {
		fifoRead(&clone);	// remove '\n' following '\r'
	}
	Fifo accu = clone;

	fifoInvalidateWrites(&accu);
	while (fifoCanRead(&clone)) {
		const char c = fifoRead(&clone);
		switch(c) {
			case '\n':
			case '\r':	fifoCopyReadPosition(fifo,&clone);
					*fifoLine = accu;
					*state = c;
					return true;
			default  :	fifoValidateWrite(&accu);
		}
	}
	return false;
}

//SLICE
bool fifoParseLine(Fifo *fifo, Fifo *fifoLine, FifoEolStyle eolStyle) {
	Fifo clone = *fifo;
	Fifo accu = *fifo;

	fifoInvalidateWrites(&accu);
	while (fifoCanRead(&clone)) {
		const char c = fifoRead(&clone);
		switch(c) {
			case '\n':
				if (eolStyle==EOL_LF		// Unix style
				|| eolStyle==EOL_CR_OR_LF	// LPC ISP 'non-style'
				|| eolStyle==EOL_AUTO) {
					fifoCopyReadPosition(fifo,&clone);
					*fifoLine = accu;
					return true;
				}
				else fifoValidateWrite(&accu);
			break;
			case '\r':
				if (eolStyle==EOL_CR		// MAC style
				|| eolStyle==EOL_CR_OR_LF) {	// LPC ISP 'non-style'
					fifoCopyReadPosition(fifo,&clone);
					*fifoLine = accu;
					return true;
				}
				else if (eolStyle==EOL_CRLF) { 		// DOS-style
					if (fifoCanRead(&clone)
					&& fifoLookAhead(&clone)=='\n') {
						fifoRead(&clone);
						fifoCopyReadPosition(fifo,&clone);
						*fifoLine = accu;
						return true;
					}
					else ; /* end of stream before \r\n or invalid line end ? */
				}
				else if (eolStyle==EOL_AUTO) {	// remove a trailing \n if available
					if (fifoCanRead(&clone) && fifoLookAhead(&clone)=='\n')
						fifoRead(&clone);
					fifoCopyReadPosition(fifo,&clone);
					*fifoLine = accu;
					return true;
				}
				else fifoValidateWrite(&accu);	// \r is a valid character within the line
			break;

			default  :	// regular char in any mode
				fifoValidateWrite(&accu);
		}
	}
	return false;
}

//SLICE
bool fifoParseUntil(Fifo *fifo, Fifo *fifoUntil, const char *separator) {
	Fifo clone = *fifo;
	Fifo accu = *fifo;

	fifoInvalidateWrites(&accu);
	while (fifoCanRead(&clone)) {
		const char c = fifoRead(&clone);
		if (strchr(separator,c)==0) {
			fifoValidateWrite(&accu);	// 'write' one more char
		}
		else {	// separator found
			fifoCopyReadPosition(fifo,&clone);	// commit the reads
			*fifoUntil = accu;
			return true;
		}
	}
	return false;	// separator never found
}

//SLICE
bool fifoParseStringNonEmpty(Fifo *fifo, Fifo *fifoString) {
	if (fifoCanRead(fifo)) {
		*fifoString = *fifo;
		fifoSkipRead(fifo,fifoCanRead(fifo));
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseWord(Fifo *text, Fifo *fifoWord, bool (*predicate)(char)) {
	Fifo clone = *text;
	Fifo result = *text;

	fifoInvalidateWrites(&result);
	while(fifoCanRead(&clone)) {
		if (predicate(fifoLookAhead(&clone))) {
			fifoRead(&clone);
			fifoValidateWrite(&result);
		}
		else break;
	}
	if (fifoCanRead(&result)) {
		*fifoWord = result;
		fifoCopyReadPosition(text,&clone);
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseN(Fifo *text, Fifo *fifoOutput, size_t n) {
	size_t nSource = fifoCanRead(text);
	if (nSource>=n) {
		*fifoOutput = *text;
		fifoOutput->wTotal = fifoOutput->rTotal + n;
		fifoSkipRead(text,n);
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseExactString(Fifo *text, const char *word) {
	Fifo clone = *text;
	for ( ;*word; ++word)
		if (fifoCanRead(&clone) && fifoLookAhead(&clone)==*word) fifoRead(&clone);
		else return false;

	fifoCopyReadPosition(text,&clone);
	return true;
}

//SLICE
bool fifoParseBoolNamed(Fifo *text, const char *symbolFalse, const char *symbolTrue, bool *value) {
	if (fifoParseExactString(text,symbolFalse)) {
		*value = false;
		return true;
	}
	else if (fifoParseExactString(text,symbolTrue)) {
		*value = true;
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseBoolOffOn(Fifo *text, bool *value) {
	return fifoParseBoolNamed(text,"off","on",value);
}

//SLICE
bool fifoParseBoolNoYes(Fifo *text, bool *value) {
	return fifoParseBoolNamed(text,"no","yes",value);
}

//SLICE
bool fifoParseEnum(Fifo *text, const char* const* symbols, int nSymbols, int *value) {
	for (int i=0; i<nSymbols; i++) if (fifoParseExactString(text,symbols[i])) {
		*value = i;
		return true;
	}
	return false;
}

//SLICE
bool fifoParseBool(Fifo *text, bool *value) {
	return fifoParseBoolNamed(text,"false","true",value);
}

//SLICE
bool fifoParseHexN(Fifo *fifo, unsigned *value, int minDigits, int maxDigits) {
	Fifo clone = *fifo;
	unsigned tValue = 0;
	unsigned digits = 0;	// successfully read digits
	while(fifoCanRead(&clone) && digits<maxDigits) {
		const char c = fifoLookAhead(&clone);	// read, but don't advance
		if (isHexDigit(c)) {
			tValue = 16*tValue + baseNDigitToInt(c);
			digits ++;
			fifoRead(&clone);
		}
		else if (isLegible (c)) fifoRead (&clone);	// just skip it
		else break;
	}

	if (minDigits<=digits) {
		*value = tValue;
		fifoCopyReadPosition(fifo,&clone);
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseHex(Fifo *fifo, unsigned *value) {
	Fifo clone = *fifo;
	unsigned tValue = 0;
	bool success = false;
	while(fifoCanRead(&clone)) {
		const char c = fifoLookAhead(&clone);	// read, but don't advance
		if (isHexDigit(c)) {
			tValue = 16*tValue + baseNDigitToInt(c);
			success = true;
			fifoRead(&clone);
		}
		else if (isLegible (c)) fifoRead (&clone);	// just skip it
		else break;
	}

	if (success) {
		*value = tValue;
		fifoCopyReadPosition(fifo,&clone);
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseHexLimited(Fifo *fifo, unsigned *value, unsigned minimum, unsigned maximum) {
	Fifo clone = *fifo;
	unsigned tValue = 0;
	bool success = false;
	while(fifoCanRead(&clone)) {
		const char c = fifoLookAhead(&clone);	// read, but don't advance
		if (isHexDigit(c)) {
			tValue = 16*tValue + baseNDigitToInt(c);
			success = true;
			fifoRead(&clone);
		}
		else if (isLegible (c)) fifoRead (&clone);	// just skip it
		else break;
	}

	if (success) {
		*value = tValue;
		fifoCopyReadPosition(fifo,&clone);
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseBin(Fifo *fifo, unsigned *value) {
	Fifo clone = *fifo;
	unsigned tValue = 0;
	bool success = false;
	while(fifoCanRead(&clone)) {
		const char c = fifoLookAhead(&clone);	// read, but don't advance
		if (isBinDigit(c)) {
			tValue = 2*tValue + baseNDigitToInt(c);
			success = true;
			fifoRead(&clone);
		}
		else if (isLegible (c)) fifoRead (&clone);	// just skip it
		else break;
	}

	if (success) {
		*value = tValue;
		fifoCopyReadPosition(fifo,&clone);
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseBinN(Fifo *fifo, unsigned *value, int minDigits, int maxDigits) {
	Fifo clone = *fifo;
	unsigned tValue = 0;
	unsigned digits = 0;	// successfully read digits
	while(fifoCanRead(&clone) && digits<maxDigits) {
		const char c = fifoLookAhead(&clone);	// read, but don't advance
		if (isBinDigit(c)) {
			tValue = 2*tValue + baseNDigitToInt(c);
			digits ++;
			fifoRead(&clone);
		}
		else if (isLegible (c)) fifoRead (&clone);	// just skip it
		else break;
	}

	if (minDigits<=digits) {
		*value = tValue;
		fifoCopyReadPosition(fifo,&clone);
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseUnsigned(Fifo *fifo, unsigned *value) {
	Fifo clone = *fifo;
	unsigned tValue = 0;
	bool success = false;
	while(fifoCanRead(&clone)) {
		const char c = fifoLookAhead(&clone);	// read, but don't advance
		if (isDigit(c)) {
			tValue = 10*tValue + c - '0';
			success = true;
			fifoRead(&clone);
		}
		else if (isLegible (c)) fifoRead (&clone);	// just skip it
		else break;
	}

	if (success) {
		*value = tValue;
		fifoCopyReadPosition(fifo,&clone);
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseUnsignedLimited(Fifo *fifo, unsigned *value, unsigned minimum, unsigned maximum) {
	Fifo clone = *fifo;
	unsigned tValue = 0;
	bool success = false;
	while(fifoCanRead(&clone)) {
		const char c = fifoLookAhead(&clone);	// read, but don't advance
		if (isDigit(c)) {
			tValue = 10*tValue + c - '0';
			success = true;
			fifoRead(&clone);
		}
		else if (isLegible (c)) fifoRead (&clone);	// just skip it
		else break;
	}

	if (success && minimum<=tValue && tValue<=maximum) {
		*value = tValue;
		fifoCopyReadPosition(fifo,&clone);
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseInt(Fifo *fifo, int *value) {
	Fifo clone = *fifo;
	unsigned absValue;
	if (fifoParseExactChar(&clone,'-')) {
		if (fifoParseUnsigned(&clone,&absValue)) {
			*value = - absValue;
			fifoCopyReadPosition(fifo,&clone);
			return true;
		}
		else return false;
	}
	else if (fifoParseUnsigned(&clone,&absValue)) {
		*value = absValue;
		fifoCopyReadPosition(fifo,&clone);
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseIntLimited(Fifo *fifo, int *value, int minimum, int maximum) {
	Fifo clone = *fifo;
	int tValue;
	if (fifoParseInt(&clone,&tValue) && minimum<=tValue && tValue<=maximum) {
		*value = tValue;
		fifoCopyReadPosition(fifo,&clone);
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseIntCStyle(Fifo *fifo, int *value) {
	Fifo clone = *fifo;
	if (fifoParseExactString (&clone,"0x") && fifoParseHex (&clone,(unsigned*)value)
	|| fifoParseExactString (&clone,"0b") && fifoParseBin (&clone,(unsigned*)value) ) { 
		fifoCopyReadPosition(fifo,&clone);
		return true;
	}
	else return fifoParseInt (fifo,value);
}

//SLICE
bool fifoParseIntEng(Fifo *fifo, int *value) {
	Fifo clone = *fifo;
	if (fifoParseIntCStyle(&clone,value)) {
		if (fifoParseExactString(&clone,"ki")) *value <<= 10;
		else if (fifoParseExactString(&clone,"Mi")) *value <<= 20;
		else if (fifoParseExactString(&clone,"Gi")) *value <<= 30;
		else if (fifoParseExactChar(&clone,'k')) *value *= 1000;
		else if (fifoParseExactChar(&clone,'M')) *value *= 1000*1000;
		else if (fifoParseExactChar(&clone,'G')) *value *= 1000*1000*1000;
		fifoCopyReadPosition(fifo,&clone);
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseTechnicalUnit(Fifo *text, int *power, const char *unit) {
	Fifo clone = *text;

	if (fifoCanRead(&clone)) switch(fifoRead(&clone)) {
		case 'y': *power = -24; break;		// yokto
		case 'z': *power = -21; break;		// zepto
		case 'a': *power = -18; break;		// atto
		case 'f': *power = -15; break;		// femto
		case 'p': *power = -12; break;		// pico
		case 'n': *power = -9; break;		// nano
		case 'u': *power = -6; break;		// micro
		case 'm': *power = -3; break;		// milli
		case 'k': *power = 3; break;		// kilo
		case 'M': *power = 6; break;		// Mega
		case 'G': *power = 9; break;		// Giga
		case 'T': *power = 12; break;		// Tera
		case 'P': *power = 15; break;		// Peta
		case 'E': *power = 18; break;		// Exa
		case 'Z': *power = 21; break;		// Zepta
		case 'Y': *power = 24; break;		// Yotta
		default :
			*power = 0;
			clone = *text;	// restore clone - no prefix
	}
	if (*unit==0
	|| fifoParseExactString(&clone,unit)) {
		fifoCopyReadPosition(text,&clone);
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseFloatMantissa(Fifo *text, float *f) {
	Fifo clone = *text;
	int sign = 1;
	*f = 0;

	// check sign
	if (fifoCanRead(&clone)) switch(fifoLookAhead(&clone)) {
		case '-': sign = -1; fifoRead(&clone); break;
		case '+': sign = +1; fifoRead(&clone); break;
		default : sign = +1; 
	}

	bool decimalDot = false;
	float fraction = 0.1f;
	bool success = false;
	for (; fifoCanRead(&clone); fifoRead(&clone)) {
		const char c = fifoLookAhead(&clone);
		if (isDigit(c)) {
			success = true;
			if (decimalDot) {
				*f += (float)sign*(c-'0')*fraction;
				fraction /= 10;
			}
			else *f = 10.f**f + sign*(c-'0');
		}
		else if (c=='.') decimalDot = true;
		else if (isLegible (c)) ;	// just skip it
		else break;
	}
	if (success) {
		fifoCopyReadPosition(text,&clone);
		return true;
	}
	else return false;
}

bool fifoParseTechnicalFloat(Fifo *text, float *f, const char *unit) {
	Fifo clone = *text;

	int extraExponent = 0;
	if (fifoParseFloatMantissa(&clone,f) && fifoParseTechnicalUnit(&clone,&extraExponent,unit)) {
		for (int i=0; i<extraExponent; ++i) *f *= 10.f;
		for (int i=0; i<-extraExponent; ++i) *f /= 10.f;
		fifoCopyReadPosition(text,&clone);
		return true;
	}
	else return false;
}

//SLICE
// this function had a bug for a long time: for high scale values, the precision is lost
static bool fifoParseFractionInt(Fifo *text, int *value, unsigned scale) {
	if (fifoParseExactChar(text,'.')) {
		int v = 0;
		int fraction = 1;
		int d;
		while (fifoParseDigit(text,&d)) {
			if (fraction*10<=scale) {
				v = v * 10 + d;
				fraction *= 10;
			}
		}
		*value =  int64Div ((Int64)v * scale,fraction);
		return true;
	};
	return false;
}

bool fifoParseFixedPointInt(Fifo *text, int *value, unsigned scale) {
	int integral = 0;
	int fractional = 0;

	// consume sign first
	int sign = 1;
	char c;
	if (fifoParseCharSet(text,&c,"+-") && (c=='-')) sign = -1;

	// accept integer + optional fraction
	if (fifoParseInt(text,&integral)) {
		fifoParseFractionInt(text,&fractional,scale);
		*value = sign * (integral*scale+fractional);
		return true;
	}
	else if (fifoParseFractionInt(text,&fractional,scale)) {	// accept .1 as value, too.
		*value = sign * (integral*scale+fractional);
		return true;
	}
	else return false;
}

//SLICE
const char* fifoReadPositionToString(Fifo *fifo) {
	return &fifo->buffer[fifo->rPos];
}

//SLICE
bool fifoPartialMatch(Fifo *fifo, const char *pattern) {
	if (pattern[0]==0) return true;

	for (int i=0; fifoCanReadRelative(fifo,i); ++i)	if (fifoLookAheadRelative(fifo,i)!=pattern[i]) return false;
	return true;
}

//SLICE
bool fifoSearch(Fifo *fifo, const char *pattern) {
	if (*pattern==0) return true;	// empty pattern always matches

	if (fifoParseExactString(fifo,pattern)) return true;
	else {
		while (!fifoPartialMatch(fifo,pattern)) fifoRead(fifo);
		return false;
	}
}

//SLICE
bool fifoMatchUntilPattern(Fifo *fifo, const char *pattern) {
	// stupid implementation :o)
	Fifo clone = *fifo;
	while (fifoCanRead(&clone)) {
		if (fifoParseExactString(&clone,pattern)) {
			fifoCopyReadPosition(fifo,&clone);
			return true;
		}
		(void)fifoRead(&clone);
	}
	return false;
}

//SLICE
bool fifoContainsPattern(Fifo *fifo, const char *pattern) {
	// stupid implementation :o)
	Fifo clone = *fifo;
	while (fifoCanRead(&clone)) {
		if (fifoParseExactString(&clone,pattern)) return true;
		(void)fifoRead(&clone);
	}
	return false;
}

//SLICE
bool fifoContainsChar(Fifo *fifo, char c) {
	for (int offset=0; offset<fifoCanRead(fifo); ++offset) {
		if (c==fifoLookAheadRelative(fifo,offset)) return true;
	}
	return false;
}

//SLICE
bool fifoMatchUntilChar(Fifo *fifo, char c) {
	for (int offset=0; offset<fifoCanRead(fifo); ++offset) {
		if (c==fifoLookAheadRelative(fifo,offset)) {
			fifoSkipRead(fifo,offset+1);
			return true;
		}
	}
	return false;
}

