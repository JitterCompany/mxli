/*
  lm73.h 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef lm73_h
#define lm73_h

/** @file
 * @brief Generic LM73 functionality on top of seqI2c.
 *
 * One Seqio object per LM73 must be provided by the user. Communication should be performed asynchronously using the
 * module seqio by the user. This generic module cannot perform the actual (hardware-dependend) IO. This must be done
 * by the user with asynchronous IO or some kind of threads (e.g. threadUs).
 * Currently only continuous temperature acquisition is supported.
 * This module used to use the I2cSeqio structure. Not anymore - this proved to be to clumsy.
 */

#include <seqio.h>

enum Lm73Configuration {
	LM73_CONFIG_RES025,		///< resolution: 1/4 degree
	LM73_CONFIG_RES0125,		///< resolution: 1/8 degree 
	LM73_CONFIG_RES00625,		///< resolution: 1/16 degree 
	LM73_CONFIG_RES003125,		///< resolution: 1/32 degree
};

enum Lm73I2cAddresses {
	// SMD code T730 - LM73-0
	LM73_ADDRESS_FLOATING	=0x48,		///< addr pin floating
	LM73_ADDRESS_GND	=0x49,		///< addr pin tied to ground
	LM73_ADDRESS_VCC	=0x4A,		///< addr pin tied to Vcc (untested)
	LM73_ADDRESS_GTS03	=LM73_ADDRESS_GND, ///< LM73 as used in AEGMIS GTS03 sensor.

	// SMD code T731 - LM73-1
	LM73_1_ADDRESS_FLOATING	=0x4C,		///< addr pin floating
	LM73_1_ADDRESS_GND	=0x4D,		///< addr pin tied to ground
	LM73_1_ADDRESS_VCC	=0x4E,		///< addr pin tied to Vcc
};

/** Blocks until the given transaction is completed or has failed.
 * @param seqio the transaction object used for a given LM73.
 * @return true for successfull completion, false for failure.
 */
bool lm73Sync(Seqio *seqio);

/** Configures LM73 for the given temperature resolution.
 * @param seqio the transaction object used for this LM73. Must be 5*4 bytes in buffer size at least.
 * @param configuration the configuration parameters.
 * @return true, if the transaction could be enqueued.
 */
bool lm73Init(Seqio *seqio, int configuration);

/** Checks, if LM73 temperature readout succeeded and updates the temperature variable accordingly.
 * @param seqio the LM73 transaction object. This is reused for issuing further temperature readouts.
 * @param tK_e10 the place to store the most recently read temperature, unit is Kelvins.
 * @return true in case of a temperature update, false otherwise.
 */
bool lm73TemperaturePoll(Seqio *seqio, int *tK_e10);

/* Prepares a new temperature readout.
 */
bool lm73TemperatureQuery(Seqio *seqio);

#endif
