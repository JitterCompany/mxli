#include <lm73.h>
/*
  lm73.c
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */


/** Addresses of LM73 registers. Low Level stuff.
 */
enum Lm73Registers {
	LM73_REG_TEMPERATURE	=0,
	LM73_REG_CONFIGURATION	=1,
	LM73_REG_THIGH		=2,
	LM73_REG_TLOW		=3,
	LM73_REG_CONTROL_STATUS	=4,
	LM73_REG_IDENTIFICATION	=5,
};

/** Bit patterns for Lm73 configuration. I do not support ALERT functionality.
 */
enum {
	LM73_CONFIGURATION_RESERVED	=1<<6,
	LM73_CONFIGURATION_POWER_DOWN	=1<<7 | 1<<6,		// LM73 power down
	LM73_CONFIGURATION_CONTINUOUS	=0<<7 | 1<<6,		// LM73 continuous conversion
	LM73_CONFIGURATION_ONE_SHOT	=1<<7 | 1<<2,
	// there are more...
};

bool lm73Sync(Seqio *seqio) {
	while (!seqioIsDone(seqio)) ;	// wait

	return seqioIsSuccessful(seqio);
}


bool lm73Init(Seqio *seqio, int configuration) {
	// 1. set continuous mode (configuration register)
	// 2. set temperature resolution (control register)
	// 3. select temperature register

//	if (!seqioIsDone(seqio)) return false;	// just to be sure...

	seqioReset(seqio);
	return
		seqioPrepareWrite(seqio, LM73_REG_CONFIGURATION | LM73_CONFIGURATION_CONTINUOUS<<8, 2,true)
		&& seqioPrepareWrite(seqio, LM73_REG_CONTROL_STATUS | (3&configuration)<<5+8, 2,true)
		&& seqioPrepareWrite(seqio, LM73_REG_TEMPERATURE, 1, true)
		&& seqioPrepareRead(seqio, 2, false)		// merge with previous write
		&& seqioPrepareEnd(seqio);
}

bool lm73TemperaturePoll(Seqio *seqio, int *tK_e10) {
	if (seqioIsJustDone(seqio) && seqioIsSuccessful(seqio)) {
		int tF128 = 0;
		if (seqioCanRead(seqio)) tF128 = (Int8)seqioRead(seqio) << 8;	// sign extend
		if (seqioCanRead(seqio)) tF128 |= (Uint8)seqioRead(seqio);	// NO sign extension!!
		*tK_e10 = (tF128<<3) + 273160*1024/1000;
		return true;
	}
	else return false;
}

bool lm73TemperatureQuery(Seqio *seqio) {

	seqioReset(seqio);
	// trigger next query
	return
		seqioPrepareWrite(seqio, LM73_REG_TEMPERATURE, 1, true)
		&& seqioPrepareRead(seqio, 2, true)
		&& seqioPrepareEnd(seqio);
}

