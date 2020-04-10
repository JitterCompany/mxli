//HEADER
#include <fifoParseStructure.h>
enum ListParseResult {
	LPR_OVERFLOW	= -2,
	LPR_INVALID	= -1,
	LPR_EMPTY	= 0,
};

//SLICE
bool fifoParse(Fifo *fifo, const FifoParse *p) {
	switch(p->type) {
		case FIFO_PARSE_TYPE_PRIMITIVE:	return p->function(fifo);
		case FIFO_PARSE_TYPE_SEQUENCE: return fifoParseSequence(fifo,p->ps);
		case FIFO_PARSE_TYPE_ALTERNATIVE: return fifoParseAlternative(fifo,p->ps);
		case FIFO_PARSE_TYPE_OPTIONAL: return fifoParseOptional(fifo,p->ps);
		default: return false;
	}
}

//SLICE
bool fifoParseSequence(Fifo *fifo, const FifoParse* const* ps) {
	Fifo clone = *fifo;
	while (*ps) if (!fifoParse(&clone,*ps)) return false;

	fifoCopyReadPosition(fifo,&clone);
	return true;
}

//SLICE
bool fifoParseAlternative(Fifo *fifo, const FifoParse* const* ps) {
	while (*ps) if (fifoParse(fifo,*ps)) return true;
	return false;
}

//SLICE
bool fifoParseOptional(Fifo *fifo, const FifoParse* const* ps) {
	while (*ps) if (!fifoParse(fifo,*ps)) return true;
	return true;
}

//SLICE
bool fifoParsePairInt(Fifo *fifo, const FifoParsePairInt *parser, PairInt *p) {
	Fifo clone = *fifo;
	PairInt pi;
	if (parser->parseIntA(&clone,&pi.a)
	&& fifoParseExactString(&clone,parser->separator)
	&& parser->parseIntB(&clone,&pi.b)
	&& (!parser->validator || parser->validator(&clone))) {
		fifoCopyReadPosition(fifo,&clone);
		*p = pi;
		return true;
	}
	else return false;
}

//SLICE
int fifoParseListInt(Fifo *fifo, const FifoParseListInt *parser, int *is, int nMax) {
	Fifo clone = *fifo;
	for (int i=0; i<nMax; ++i) {
		if (i>0) {
			if (fifoParseExactString(&clone,parser->separator)) { // list must be continued
				if (parser->parseInt(&clone,&is[i])) ;
				else return LPR_INVALID;	// list cannot be terminated by separator
			}
			else {	// end of list
				if (!parser->validator || parser->validator(&clone)) {
					fifoCopyReadPosition(fifo,&clone);
					return i;
				}
				else return LPR_INVALID;
			}
		}
		else {	// first element at all
			if (parser->parseInt(&clone,&is[i])) ;
			else	if (!parser->validator || parser->validator(&clone)) return 0;	// empty list
				else return LPR_INVALID;
		}
	}
	// check for end of list
	if (!parser->validator || parser->validator(&clone)) {
		fifoCopyReadPosition(fifo,&clone);
		return nMax;
	}
	else return LPR_OVERFLOW;
}

//SLICE
int fifoParseListPairInt(Fifo *fifo, const FifoParseListPairInt* parser, PairInt *ps, int nMax) {
	Fifo clone = *fifo;
	for (int i=0; i<nMax; ++i) {
		if (i>0) {
			if (fifoParseExactString(&clone,parser->separator)) { // list must be continued
				if (fifoParsePairInt(&clone,parser->parsePair,&ps[i])) ;
				else return LPR_INVALID;	// list cannot be terminated by separator
			}
			else {	// end of list
				if (!parser->validator || parser->validator(&clone)) {
					fifoCopyReadPosition(fifo,&clone);
					return i;
				}
				else return LPR_INVALID;
			}
		}
		else {	// first element at all
			if (fifoParsePairInt(&clone,parser->parsePair,&ps[i])) ;
			else	if (!parser->validator || parser->validator(&clone)) return 0;	// empty list
				else return LPR_INVALID;
		}
	}
	// check for end of list
	if (!parser->validator || parser->validator(&clone)) {
		fifoCopyReadPosition(fifo,&clone);
		return nMax;
	}
	else return LPR_OVERFLOW;
}

//SLICE
int fifoParseHex32Masked(Fifo *fifo, Uint32 *result, Uint32 *mask, char dontCare) {
	Fifo clone = *fifo;
	Uint32 r=0, m=0;
	int digit;
	int digits;
	if (!fifoParseExactString(fifo,"0x")) return 0;

	for (digits=0; digits<8; digits++) {
		if (fifoParseHexDigit(&clone,&digit)) {
			r = r<<4 | digit;
			m = m<<4 | 0xF;
		}
		else if (fifoParseExactChar(&clone,dontCare)) {
			r <<=4;
			m <<=4;
		}
		else break;	// end of value
	}
	fifoCopyReadPosition(fifo,&clone);
	return digits+2;
}

