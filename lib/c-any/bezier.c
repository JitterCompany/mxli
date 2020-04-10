//HEADER
#include <bezier.h>

//SLICE
bool cubicIteratorNext (CubicIterator *it) {
	Uint32 next = it->t_e32 + it->dt_e32;
	if (next > it->t_e32) {
		it->t_e32 = next;
		it->t2_e32 = (Uint64)next * next >> 32;
		it->t3_e32 = (Uint64)next * it->t2_e32 >> 32;
		return true;
	}
	else return false;
}

//SLICE
Int32 int32Poly3CubicIteratorValue (const Int32 *poly, const CubicIterator *it) {
	return	(((Int64)poly[0]<<32)
		+poly[1]*(Int64)cubicIteratorT_e32 (it)
		+poly[2]*(Int64)cubicIteratorT2_e32 (it)
		+poly[3]*(Int64)cubicIteratorT3_e32 (it)
		) >> 32;
}

//SLICE
Int32 int32Poly3CubicIteratorSpeed (const Int32 *poly, const CubicIterator *it) {
	return	(((Int64)poly[1]<<32)
		+2*poly[2]*(Int64)cubicIteratorT_e32 (it)
		+3*poly[3]*(Int64)cubicIteratorT2_e32 (it)
		) >> 32;
}

//SLICE
Int32 int32Poly3CubicIteratorAcceleration (const Int32 *poly, const CubicIterator *it) {
	return	((2*(Int64)poly[2]<<32)
		+6*poly[3]*(Int64)cubicIteratorT_e32 (it)
		) >> 32;
}

//SLICE
Int32 int32Poly3CubicIteratorJerk (const Int32 *poly) {
	return	6*poly[3];
}

////////////////////////////////////////////////////////////////////////////////////////////////////


//SLICE
void cubicIncrementalIteratorStart (CubicIncrementalIterator *it, Uint32 dt_e32) {
	it->dt_e32 = dt_e32;
	it->dt2_e64 = (Uint64)dt_e32 * dt_e32;
	it->dt3_e64 = uint64FractionCube (dt_e32);

	it->t_e32 = 0;

	it->t2_e64[1] = 0;
	it->t2_e64[0] = it->dt2_e64;

	it->t3_e64[2] = 0;
	it->t3_e64[1] = it->dt3_e64;
	it->t3_e64[0] = 8*it->dt3_e64;
}

//SLICE
bool cubicIncrementalIteratorNext (CubicIncrementalIterator *it) {
	const Uint32 next = it->t_e32 + it->dt_e32;
	if (next >= it->t_e32) {
		// t^1
		it->t_e32 = next;
		// t^2
		Uint64 new = (it->t2_e64[0] - it->t2_e64[1]) + it->t2_e64[0] + 2*it->dt2_e64;
		it->t2_e64[1] = it->t2_e64[0];
		it->t2_e64[0] = new;
		// t^3
		new = 3 * (it->t3_e64[0] - it->t3_e64[1]) + it->t3_e64[2] + 6*it->dt3_e64;
		it->t3_e64[2] = it->t3_e64[1];
		it->t3_e64[1] = it->t3_e64[0];
		it->t3_e64[0] = new;
		return true;
	}
	else return false;
}

//SLICE
Int32 int32Poly3CubicIncrementalIteratorValue (const Int32 *poly, const CubicIncrementalIterator *it) {
	return	(((Int64)poly[0]<<32)
		+poly[1]*(Int64)cubicIncrementalIteratorT_e32 (it)
		+poly[2]*(Int64)cubicIncrementalIteratorT2_e32 (it)
		+poly[3]*(Int64)cubicIncrementalIteratorT3_e32 (it)
		) >> 32;
}

//SLICE
/** Linear changing acceleration
 */
void int32Bezier3ToPoly3 (Int32 x0, Int32 x1, Int32 x2, Int32 x3, Int32 *poly) {
	poly[0] = x0;
	poly[1] = 3*x1-3*x0;
	poly[2] = 3*x2 - 6*x1 + 3*x0;
	poly[3] = x3 - x0 + 3*x1 - 3*x2;
}

//SLICE
/** Constant acceleration curve
 */
void int32Bezier2ToPoly3 (Int32 x0, Int32 x1, Int32 x2, Int32 *poly) {
	poly[0] = x0;
	poly[1] = 2*x1-2*x0;
	poly[2] = x2 - 2*x1 + x0;
	poly[3] = 0;
}

