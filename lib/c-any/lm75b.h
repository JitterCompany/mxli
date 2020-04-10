/*
  lm75b.h 
  Copyright 2015 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef lm75b_h
#define lm75b_h

/** @file
 * @brief Generic LM75B (enhanced LM75) functionality on top of seqI2c.
 *
 * One Seqio object per LM75 must be provided by the user. Communication should be performed asynchronously using the
 * module seqio by the user. This generic module cannot perform the actual (hardware-dependend) IO. Currently only
 * continuous temperature acquisition is supported. This module uses LM75 with its reset defaults so no initialization
 * of LM75B is required.
 *
 * LM75B (NXP) is an enhanced LM75 with 0.125K resolution and the possibility of powering down the device through a
 * command.
 */

#include <seqio.h>

/** Blocks until the given transaction is completed or has failed.
 * @param seqio the transaction object used for a given LM75.
 * @return true for successfull completion, false for failure.
 */
bool lm75bSync(Seqio *seqio);

/** Checks, if LM75 temperature readout succeeded and updates the temperature variable accordingly.
 * @param seqio the LM75 transaction object. This is reused for issuing further temperature readouts It must be at
 *   least two elements in size (for reading 16 bits of temperature).
 * @param temperature_e10 the place to store the most recently read temperature.
 * @return true in case of a temperature update, false otherwise.
 */
bool lm75bTemperaturePoll(Seqio *seqio, int *temperature_e10);

/* Prepares a new temperature readout.
 * @param seqio the LM75 transaction object. This is reused for issuing further temperature readouts It must be at
 *   least two elements in size (for reading 16 bits of temperature).
 */
bool lm75bTemperatureQuery(Seqio *seqio);

#endif
