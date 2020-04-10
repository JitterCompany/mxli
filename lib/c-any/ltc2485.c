//HEADER
#include <ltc2485.h>

//SLICE
bool ltc2485PrepareConfig (Seqio *seqio, int configuration) {
	return seqioPrepareWrite (seqio,configuration,1,true);
}

//SLICE
bool ltc2485PrepareConfigThenReadThenTrigger (Seqio *seqio, int configuration) {
	return  seqioPrepareWrite (seqio,configuration,1,false)
		&& seqioPrepareRead (seqio,3,false)
		&& seqioPrepareRead (seqio,1,false)
		;	// STOP causes conversion
}

//SLICE
bool ltc2485PrepareReadThenTrigger (Seqio *seqio) {
	// just a read of 4 bytes
	return  seqioPrepareRead (seqio,2,false)
		&& seqioPrepareRead (seqio,2,false)
		;	// STOP causes conversion
}

//SLICE
bool ltc2485Extract_e24 (Seqio *seqio, Int32 *raw) {
	if (seqioIsSuccessful (seqio)
	&& seqioCanRead (seqio)) {
		Uint32 bits	= (Uint32)seqioActionRead (seqio) << 24
				| (Uint32)seqioActionRead (seqio) << 16
				| (Uint32)seqioActionRead (seqio) << 8
				| (Uint32)seqioActionRead (seqio) & 0xC0;	// truncate the noise

		const bool over = (bits>>30) == 3;
		const bool under = (bits>>30) == 0;
		//const Int32 sign = bits & 1<<31 ? 1 : -1;		// indicates POSITIVE
		const Int32 value = (Int32)bits<<1 >>7;			// 2's complement
		*raw = over ? (1u<<31)-1 : (under ? (1<<31): value);

		return true;
	}
	else return false;
}

//SLICE
bool ltc2485ExtractV_e24 (Seqio *seqio, Int32 vRef_e24, Int32 *v_e24) {
	Int32 raw_e24;

	if (ltc2485Extract_e24 (seqio,&raw_e24)) {
		// FS = vRef / 2
		// formula: v_e24 = raw_e24 / E24 * vRef_e24 / 2
		*v_e24 = (Int64)raw_e24 * vRef_e24 >> 25;
		return true;
	}
	else return false;
}

//SLICE
bool ltc2485ExtractK_e10 (Seqio *seqio, Int32 vRef_e24, Int32 *k_e10) {
	Int32 v_e24;

	if (ltc2485ExtractV_e24 (seqio,vRef_e24,&v_e24)) {
		// FS = vRef / 2
		// internal PTAT: 420mV @ 300K (27C) = 1.4mV / K = 0.71428 K / mV = 714.2857 K / V = 2857.14/4 K / V
		// formula: k_e24 = raw_e24 / E24 * vRef_e24 / 2
		*k_e10 = (v_e24>>5) * 2857 >> 11;
		return true;
	}
	else return false;
}

