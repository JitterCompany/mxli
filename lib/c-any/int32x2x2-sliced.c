//HEADER
#include <int32x2x2.h>

//SLICE
Int32x2x2 int32x2x2DirectionScalingMatrix_en (int e, Int32x2 v) {
	Int32x2 vUnit = int32x2ScaleTo1_en (e,v);
	return	int32x2x2MulTripleInt32x2x2_en (e,
			int32x2x2FromComplexConj (vUnit),
			int32x2x2ScalingMatrixXy (int32x2Abs (v),1<<e),
			int32x2x2FromComplex (vUnit)
		);
}

//SLICE
Int32x2x2 int32x2x2Invert_en (int e, Int32x2x2 a) {
	const Int32 det = int32x2x2Det_en (e,a);
	const Int32x2x2 r = {		int32Div_en (e,a.yy,det),	int32Div_en (e,-a.yx,det),
					int32Div_en (e,-a.xy,det),	int32Div_en (e,a.xx,det)	};
	return r;
}

//SLICE
Int32x2 int32x2Affine_en (int e, const Int32x2Affine affine, Int32x2 x) {
	return int32x2Add (
		int32x2x2MulInt32x2_en (e,affine.matrix,x),
		affine.shift
	);
}

//SLICE
Int32x2Affine int32x2AffineInvert_en (int e, const Int32x2Affine affine) {
	const Int32x2x2 inverseA = int32x2x2Invert_en (e,affine.matrix);

	const Int32x2Affine inverse = {
		inverseA,
		int32x2x2MulInt32x2_en (e, inverseA, int32x2jj (affine.shift))
	};
	return inverse;	
}

