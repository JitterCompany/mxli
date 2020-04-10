/*
  lm92.h 
  Copyright 2015 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef lm92_h
#define lm92_h

/** @file
 * @brief Generic LM92 functionality on top of seqI2c.
 *
 * One Seqio object per LM92 must be provided by the user. Communication should be performed asynchronously using the
 * module seqio by the user. This generic module cannot perform the actual (hardware-dependend) IO. Currently only
 * continuous temperature acquisition is supported. LM92 provides 16-bit temperature, with the lowest 3 bits being
 * flags, rather than temperature. D3=1/16=0.0625C i.e. temperature readout is _e7
 */

#include <i2cSeqio.h>

/** LM92 can be configured for one of the following 4 I2C addresses. Up to 4 LM92 are possible on one bus.
 */
enum Lm92Constants {
	LM92_ADDRESS_0		=0x48,		///< A0=0, A1=0
	LM92_ADDRESS_1		=0x49,		///< A0=1, A1=0
	LM92_ADDRESS_2		=0x4A,		///< A0=0, A1=1
	LM92_ADDRESS_3		=0x4B,		///< A0=1, A1=1
	LM92_VENDOR_ID		=0x8001,	///< probably to distinguish from other LMs at same I2C addresses.
	LM92_TEMPERATURE_MASK	=0xFFF8,	///< mask off the flags D0..D2
	LM92_E			=7,		///< position of the decimal point: 1<<7
};

/** Blocks until the given transaction is completed or has failed.
 * @param seqio the transaction object used for a given LM92.
 * @return true for successfull completion, false for failure.
 */
bool lm92Sync(I2cSeqio *seqio);

/** Configures LM92. LM92's reset configuration does not require a configuration, so this function is optional on
 * startup. The purpose of it is to provide power on/off of LM92.
 * @param seqio the transaction object used for this LM92. Must be 5*4 bytes in buffer size at least.
 * @param powerOn true to enable LM92 (reset default), false to shut it down.
 * @return true, if the transaction could be enqueued.
 */
bool lm92Init(I2cSeqio *seqio, bool powerOn);

/** Checks, if LM92 temperature readout succeeded and updates the temperature variable accordingly.
 * @param seqio the LM92 transaction object. This is reused for issuing further temperature readouts.
 * @param tK_e10 the place to store the most recently read temperature (in Kelvins!).
 * @return true in case of a temperature update, false otherwise.
 */
bool lm92TemperaturePoll(I2cSeqio *seqio, int *tK_e10);

/* Prepares a new temperature readout.
 */
bool lm92TemperatureQuery(I2cSeqio *iseqio);

#endif
