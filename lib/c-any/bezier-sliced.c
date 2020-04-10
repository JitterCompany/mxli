//HEADER
#include <bezier.h>
#include <int32Math.h>
#include <int64Math.h>

//SLICE
Int32 int32Bezier2GlobalExtreme (Int32 x0, Int32 x1, Int32 x2) {
	const Int32 n = (x0-x1) + (x2-x1);
	const Int64 z = (Int64)x0*x2 - (Int64)x1*x1;
	return int64Div (z,n);
}

//SLICE
Int32 int32Bezier2Min (Int32 x0, Int32 x1, Int32 x2) {
	if (x0-x1 > 0 && x2-x1 > 0) {	// extreme condition, and we know from the sum (acceleration/2) it's a minimum
					// between x0 and x2
		return int32Bezier2GlobalExtreme (x0,x1,x2);
	}
	else return int32Min (x0,x2);
}

//SLICE
Int32 int32Bezier2Max (Int32 x0, Int32 x1, Int32 x2) {
	if (x0-x1 < 0 && x2-x1 < 0) {	// extreme condition, and we know from the sum (acceleration/2) it's a maximum
					// between x0 and x2
		return int32Bezier2GlobalExtreme (x0,x1,x2);
	}
	else return int32Max (x0,x2);
}

