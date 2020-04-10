//HEADER
#include <int32PairList.h>

//SLICE
bool int32PairListCopy(Int32PairList *dst, const Int32PairList *src) {
	int32PairListClear(dst);
	bool success = true;
	for (int i=0; i<int32PairListLength(src); i++) success = success && int32PairListAdd (dst, int32PairListAt(src,i));
	return success;
}

//SLICE
int int32PairListTransitiveFusion (Int32PairList *list) {
	int fusions = 0;
	int w=0, r=1;
	while (r<int32PairListLength(list)) {
		if (list->elements[w].snd == list->elements[r].fst) {
			list->elements[w].snd = list->elements[r].snd;
			fusions --;
		}
		else {
			w++;
			list->elements[w] = list->elements[r];
		}
		r++;
	}
	list->length += fusions * sizeof (Int32Pair);
	return fusions;
}

//SLICE
int int32PairListSequenceFusion (Int32PairList *list) {
	int fusions = 0;
	int w=0, r=1;
	while (r<int32PairListLength(list)) {
		if (list->elements[w].snd+1 == list->elements[r].fst) {
			list->elements[w].snd = list->elements[r].snd;
			fusions --;
		}
		else {
			w++;
			list->elements[w] = list->elements[r];
		}
		r++;
	}
	list->length += fusions * sizeof (Int32Pair);
	return fusions;
}

//SLICE
bool fifoPrintInt32PairList (Fifo *fifo, const Int32PairList *list, bool (*printPair)(Fifo *, Int32Pair const*), char separator) {
	bool success = true;
	for (int i=0; success && i<int32PairListLength (list); i++) {
		if (i!=0) success = success && fifoPrintChar (fifo,separator);
		success = success && printPair (fifo,int32PairListAt (list,i));
	}
	return success;
}

//SLICE
int fifoParseInt32PairList (Fifo *fifo, Int32PairList *list, bool (*parseElement)(Fifo *fifo, Int32Pair*), char separator) {
	Fifo clone = *fifo;
	const int n0 = int32PairListLength(list);
	const int nMax = int32PairListSize(list) - n0;
	Int32Pair bufferList [ nMax ];
	Int32PairList l = { bufferList, sizeof bufferList, };
	int n=0;
	while (n<nMax) {
		Int32Pair pair;
		if (fifoCanRead(&clone)					// end of input
		&& (n==0 || fifoParseExactChar (&clone,separator)) 	// done because of missing ','
		&& parseElement (&clone,&pair) ) {
			int32PairListAdd(&l,&pair);
			n++;
		}
		else break;
	}
	if (n>0) {
		int32PairListCopy(list,&l);	
		fifoCopyReadPosition(fifo,&clone);
	}
	return n;	// list capacity exhausted
}

//SLICE
int fifoParseInt32PairListDynamic (Fifo *fifo, Int32PairList *list, const FifoParseInt32Pair *parsePair, char separator) {
	Fifo clone = *fifo;
	const int n0 = int32PairListLength(list);
	const int nMax = int32PairListSize(list) - n0;
	Int32Pair bufferList [ nMax ];
	Int32PairList l = { bufferList, sizeof bufferList, };
	int n=0;
	while (n<nMax) {
		Int32Pair pair;
		if (fifoCanRead(&clone)					// end of input
		&& (n==0 || fifoParseExactChar (&clone,separator)) 	// done because of missing ','
		&& fifoParseInt32PairDynamic (&clone,&pair,parsePair) ) {
			int32PairListAdd(&l,&pair);
			n++;
		}
		else break;
	}
	if (n>0) {
		int32PairListCopy(list,&l);	
		fifoCopyReadPosition(fifo,&clone);
	}
	return n;	// might be limited by list capacity
}

//SLICE
int fifoParseInt32PairListWithInt32Parameter (
	Fifo *fifo, Int32PairList *list, const FifoParseInt32PairWithInt32Parameter *parser, char separator) {

	Fifo clone = *fifo;
	const int n0 = int32PairListLength(list);
	const int nMax = int32PairListSize(list) - n0;
	Int32Pair bufferList [ nMax ];
	Int32PairList l = { bufferList, sizeof bufferList, };
	int n=0;
	while (n<nMax) {
		Int32Pair pair;
		if (fifoCanRead(&clone)					// end of input
		&& (n==0 || fifoParseExactChar (&clone,separator)) 	// done because of missing ','
		&& parser->parse (&clone,&pair,parser->parameter) ) {
			int32PairListAdd(&l,&pair);
			n++;
		}
		else break;
	}
	if (n>0) {
		int32PairListCopy(list,&l);	
		fifoCopyReadPosition(fifo,&clone);
	}
	return n;	// might be limited by list capacity
}

