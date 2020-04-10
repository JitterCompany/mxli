//HEADER
#include <lm92.h>

/** Addresses of LM92 registers. Low Level stuff.
 */
enum Lm92Registers {
	LM92_REG_TEMPERATURE	=0,
	LM92_REG_CONFIGURATION	=1,
	LM92_REG_THYST		=2,
	LM92_REG_TCRIT		=3,
	LM92_REG_TLOW		=4,
	LM92_REG_THIGH		=5,
	LM92_REG_IDENTIFICATION	=7,
};

/** Bit patterns for Lm92 configuration. I do not support most of them.
 */
enum {
	LM92_CONFIGURATION_POWER_DOWN	=1<<0,		// LM92 power down
};

//SLICE
bool lm92Sync(I2cSeqio *iseqio) {
	while (!seqioIsDone(&iseqio->seqio)) ;	// wait

	return seqioIsSuccessful(&iseqio->seqio);
}


//SLICE
bool lm92Init(I2cSeqio *iseqio, bool powerUp) {
	Seqio * const seqio = &iseqio->seqio;

//	if (!seqioIsDone(seqio)) return false;	// just to be sure...

	seqioReset(seqio);
	return
		seqioPrepareWrite(seqio,
			LM92_REG_CONFIGURATION | (powerUp ? 0 : LM92_CONFIGURATION_POWER_DOWN)<<8,
			2,true
		);
}

//SLICE
bool lm92TemperaturePoll(I2cSeqio *iseqio, int *tK_e10) {
	Seqio * const seqio = &iseqio->seqio;

	bool tempUpdated = false;

	if (seqioIsJustDone(seqio)) {
		if (seqioIsSuccessful(seqio)) {
			int t_e7 = 0;
			if (seqioCanRead(seqio)) t_e7 = (Int8)seqioRead(seqio) << 8;		// sign extend
			if (seqioCanRead(seqio)) t_e7 |= (Uint8)seqioRead(seqio) & 0xF8;	// NO sign extension!!
			*tK_e10 = (t_e7<<3) + 273160*1024/1000;
			tempUpdated = true;
		}
	}

	return tempUpdated;
}

//SLICE
bool lm92TemperatureQuery(I2cSeqio *iseqio) {
	Seqio * const seqio = &iseqio->seqio;

	seqioReset(seqio);
	// trigger next query
	return
		seqioPrepareWrite(seqio, LM92_REG_TEMPERATURE, 1, true)
		&& seqioPrepareRead(seqio, 2, true)
		&& seqioPrepareEnd(seqio);
}

