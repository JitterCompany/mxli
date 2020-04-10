#include <quadratic.h>

Int32x2 int32MidnightPq_en (int e, Int32 p_en, Int32 q_en) {
	const Int32 ph_en = p_en/2;
	const Int64 w2_e2n = (Int64)ph_en * ph_en - ((Int64)q_en<<e);

	if (w2_e2n>=0) {
		const Int32 w_en = uint64SqrtFloor (w2_e2n);
		return int32x2 (-ph_en-w_en, -ph_en+w_en);
	}
	else return int32x2Undefined ();
}

Int32x2 int32MidnightAbc_en (int e, Int32 a_en, Int32 b_en, Int32 c_en) {
	// modified formula: x1/2=-b/2a +- sqrt( (b/2)^2 -ac )/a
	const Int32 bh_en = b_en/2;
	const Int64 w2_e2n = (Int64)bh_en * bh_en - ((Int64)a_en*c_en);

	if (w2_e2n>=0) {
		const Uint32 w_en = int32Div_en (e,uint64SqrtFloor (w2_e2n),a_en);
		const Int32 p_en = int32Div_en (e,b_en,a_en) / 2;
		return int32x2 (-p_en-w_en, -p_en+w_en);
	}
	else return int32x2Undefined ();
}

