//HEADER
#include <int32Pair.h>

//SLICE
bool fifoParseInt32Pair (Fifo *fifo, Int32Pair *pair, bool (*parseElement)(Fifo *fifo, Int32*), char separator) {
	Fifo clone = *fifo;
	Int32Pair pTemp;
	if (parseElement (&clone, &pTemp.fst)
	&& fifoParseExactChar (&clone, separator)
	&& parseElement (&clone, &pTemp.snd)) {
		*pair = pTemp;
		fifoCopyReadPosition (fifo, &clone);
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseInt32PairDynamic (Fifo *fifo, Int32Pair *pair, FifoParseInt32Pair const *parser) {
	Fifo clone = *fifo;
	Int32Pair pTemp;
	if (parser->parseFst (&clone, &pTemp.fst)
	&& fifoParseExactChar (&clone, parser->separator)
	&& parser->parseSnd (&clone, &pTemp.snd)) {
		*pair = pTemp;
		fifoCopyReadPosition (fifo, &clone);
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseInt32WithDontCares (Fifo *fifo, Int32Pair *pair, Int32 dontCare) {
	Fifo clone = *fifo;

	if (fifoParseExactString (&clone, "0x") && fifoParseInt32HexWithDontCares (&clone,pair,dontCare)
	|| (clone=*fifo, fifoParseExactString (&clone, "0b") && fifoParseInt32BinWithDontCares (&clone,pair,dontCare))) {
		fifoCopyReadPosition (fifo,&clone);
		return true;
	}
	else {
		Uint32 value;
		if (fifoParseIntCStyle (fifo,(Int32*)&value)) {
			pair->fst = value;
			pair->snd = -1;
			return true;
		}
		else return false;
	}
}

//SLICE
bool fifoParseInt32BinWithDontCares (Fifo *fifo, Int32Pair *value, Int32 dontCare) {
	Fifo clone = *fifo;
	Uint32 tValue = 0;
	Uint32 tMask = 0;
	bool success = false;
	while(fifoCanRead(&clone)) {
		const char c = fifoLookAhead(&clone);	// read, but don't advance
		if (isBinDigit(c)) {
			tValue = 2*tValue | baseNDigitToInt(c);
			tMask = 2*tMask | 1;
			success = true;
			fifoRead(&clone);
		}
		else if (c==dontCare) {
			tValue = 2*tValue | 0;
			tMask = 2*tMask | 0;
			success = true;
			fifoRead(&clone);
		}
		else if (isLegible(c)) fifoRead (&clone);
		else break;
	}

	if (success) {
		value->fst = tValue;
		value->snd = tMask;
		fifoCopyReadPosition(fifo,&clone);
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseInt32HexWithDontCares (Fifo *fifo, Int32Pair *value, Int32 dontCare) {
	Fifo clone = *fifo;
	Uint32 tValue = 0;
	Uint32 tMask = 0;
	bool success = false;
	while(fifoCanRead(&clone)) {
		const char c = fifoLookAhead(&clone);	// read, but don't advance
		if (isHexDigit(c)) {
			tValue = 16*tValue | baseNDigitToInt(c);
			tMask = 16*tMask | 0xF;
			success = true;
			fifoRead(&clone);
		}
		else if (c==dontCare) {
			tValue = 16*tValue | 0;
			tMask = 16*tMask | 0;
			success = true;
			fifoRead(&clone);
		}
		else if (isLegible(c)) fifoRead (&clone);
		else break;
	}

	if (success) {
		value->fst = tValue;
		value->snd = tMask;
		fifoCopyReadPosition(fifo,&clone);
		return true;
	}
	else return false;
}

//SLICE
bool canPrintInt32HexWithDontCares (const Int32Pair *pair) {
	const Uint32 mask = pair->snd;
	for (int n=0; n<8; n++) if ((mask>>4*n & 0xF) != 0 && (mask>>4*n & 0xF) != 0xF) return false;
	return true;
}

//SLICE
bool fifoPrintInt32HexWithDontCares (Fifo *fifo, const Int32Pair *pair, char dontCare) {
	const Uint32 mask = pair->snd;
	const Uint32 value = pair->fst;
	int n=7;
	while (n>0 && (mask>>4*n & 0xF)==0) n--;
	while (n>=0) {
		if ((mask>>4*n & 0xF) !=0) 
			if (!fifoPrintBaseNChar (fifo, value>>4*n & 0xF)) return false;
			else ;
		else if (!fifoPrintChar (fifo,dontCare)) return false;
		n--;
	}
	return true;
}

//SLICE
bool fifoPrintInt32BinWithDontCares (Fifo *fifo, const Int32Pair *pair, char dontCare) {
	const Uint32 mask = pair->snd;
	const Uint32 value = pair->fst;
	int n=31;
	while (n>0 && (mask>>n & 0x1)==0) n--;
	while (n>=0) {
		if ((mask>>n & 0x1) !=0)
			if (!fifoPrintBaseNChar (fifo, value>>n & 0x1)) return false;
			else ;
		else if (!fifoPrintChar (fifo,dontCare)) return false;
		n--;
	}
	return true;
}

