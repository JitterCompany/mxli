//HEADER
#include <lm75b.h>

/** Addresses of LM75 registers. Low Level stuff.
 */
enum Lm75bRegisters {
	LM75_REG_TEMPERATURE	=0,
	LM75_REG_CONFIGURATION	=1,
};

//SLICE
bool lm75bSync (Seqio *seqio) {
	while (!seqioIsDone(seqio)) ;	// wait

	return seqioIsSuccessful(seqio);
}

//SLICE
bool lm75bTemperaturePoll (Seqio *seqio, int *t_e10) {

	bool tempUpdated = false;

	if (seqioIsJustDone (seqio)) {
		if (seqioIsSuccessful(seqio)) {
			// LM75b: 11bits (5..15) of 16 with 0.125 K resolution.
			int t_e8 = 0;
			if (seqioCanRead(seqio)) t_e8 = (Int8)seqioRead(seqio) << 8;		// sign extend
			if (seqioCanRead(seqio)) t_e8 |= (Uint8)seqioRead(seqio) & 0xE0;	// NO sign extension!!
			*t_e10 = (t_e8<<2) + 273160*1024/1000;
			tempUpdated = true;
		}
	}

	return tempUpdated;
}

//SLICE
bool lm75bTemperatureQuery (Seqio *seqio) {

	seqioReset(seqio);
	// trigger next query
	return
		//seqioPrepareWrite(seqio, LM75_REG_TEMPERATURE, 1, true)
		seqioPrepareRead (seqio, 2, true)
		&& seqioPrepareEnd (seqio);
}

