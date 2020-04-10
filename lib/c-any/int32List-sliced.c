//HEADER
#include <int32List.h>

//SLICE
bool fifoPrintInt32List (Fifo *fifo, const Int32List *list, bool (*printElement)(Fifo *, Int32), char separator) {
	bool success = true;
	for (int i=0; success && i<int32ListLength (list); i++) {
		if (i!=0) success = success && fifoPrintChar (fifo,separator);
		success = success && printElement (fifo,int32ListAt (list,i));
	}
	return success;
}

//SLICE
int fifoParseInt32List (Fifo *fifo, Int32List *list, bool (*parseElement)(Fifo *fifo, Int32*), char separator) {
	Fifo clone = *fifo;
	const int n0 = int32ListLength(list);
	const int nMax = int32ListSize(list) - n0;
	int n=0;
	while (n<nMax) {
		Int32 value;
		if (fifoCanRead(&clone)						// not EOT
		&& ((n==0) || fifoParseExactChar (&clone,separator))	 	// parse , except for beginning of list
		&& parseElement (&clone,&value) ) {	// found an int
			int32ListAdd (list,value);
			n++;
		}
		else {
			fifoCopyReadPosition (fifo,&clone);
			return n;
		}
	}
	return -1;	// list capacity exhausted
}

//SLICE
static bool fifoPrintInt32Min(Fifo *fifo, Int32 value) {
	return fifoPrintSDec(fifo,value,1,11,false);
}

bool fifoPrintInt32ListDec (Fifo *fifo, Int32List *list) {
	return fifoPrintInt32List( fifo, list, fifoPrintInt32Min, ',');
}

//SLICE
static bool fifoPrintUint32Min(Fifo *fifo, Int32 value) {
	return fifoPrintUDec(fifo,value,1,11);
}

bool fifoPrintUint32ListDec (Fifo *fifo, Uint32List *list) {
	return fifoPrintInt32List( fifo, (Int32List*)list, fifoPrintUint32Min, ',');
}

//SLICE
static bool fifoPrintHex32Min(Fifo *fifo, Int32 value) {
	return fifoPrintString(fifo,"0x") && fifoPrintHex(fifo,value,1,8);
}

bool fifoPrintUint32ListHex (Fifo *fifo, Uint32List *list) {
	return fifoPrintInt32List (fifo,(Int32List*)list,fifoPrintHex32Min, ',');
}



