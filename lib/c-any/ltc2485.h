/*
  ltc2485.h 
  Copyright 2015 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef ltc2485_h
#define ltc2485_h

/** @file
 * @brief Generic LTC2485 I2C functionality on top of seqio (for execution of I2C).
 *
 * One Seqio object per LTC2485 must be provided by the user. Communication should be performed asynchronously using the
 * module seqio by the user. This generic module cannot perform the actual (hardware-dependend) IO.
 *
 * LTC2485 uses 4byte data as conversion result. Every STOP condition triggers a new conversion.
 * The worst case conversion time is 163.5ms (1x speed) or 81.9ms (2x speed). Although, LTC2485 has 25bits conversion
 * result/resolution, the offset error is in the mV range (<2mV @ 2.5V vRef).
 * The minimum size of a Seqio object for using all functions defined here is 4 elements.
 */

#include <seqio.h>

enum Ltc2485Configuration {
	_LTC2485_CONFIG_IM		=3,		///< IM bit, selects internal temperature sensor
	_LTC2485_CONFIG_REJECT60_ONLY	=2,		///< 60Hz rejection, not 50Hz
	_LTC2485_CONFIG_REJECT50_ONLY	=1,		///< 50Hz rejection, not 60Hz
	_LTC2485_CONFIG_SPD		=0,		///< double speed, no offset calibration
	LTC2485_CONFIG_IM		=1<<_LTC2485_CONFIG_IM,			///< selects internal temperature sensor
	LTC2485_CONFIG_REJECT50AND60	=0,					///< 50Hz and 60Hz rejection
	LTC2485_CONFIG_REJECT50		=1<<_LTC2485_CONFIG_REJECT50_ONLY,	///< 50Hz rejection
	LTC2485_CONFIG_REJECT60		=1<<_LTC2485_CONFIG_REJECT60_ONLY,	///< 60Hz rejection
	LTC2485_CONFIG_SPD		=1<<_LTC2485_CONFIG_SPD,		///< double speed, no offset calibration
};

enum Ltc2485I2cAddresses {
	LTC2485_ADDRESS_BASE		=0x14,		
	LTC2485_ADDRESS_CA0H_CA1L	=LTC2485_ADDRESS_BASE+0,
	LTC2485_ADDRESS_CA0F_CA1L	=LTC2485_ADDRESS_BASE+1,
	LTC2485_ADDRESS_CA0H_CA1F	=LTC2485_ADDRESS_BASE+3,
	LTC2485_ADDRESS_CA0F_CA1F	=LTC2485_ADDRESS_BASE+0x10,
	LTC2485_ADDRESS_CA0H_CA1H	=LTC2485_ADDRESS_BASE+0x12,
	LTC2485_ADDRESS_CA0F_CA1H	=LTC2485_ADDRESS_BASE+0x13,
};

/** Configures LTC2485 and starts the next conversion on the following stop.
 * @param seqio the transaction object used for this LTC2485. 
 * @param configuration the configuration parameters.
 * @return true, if the transaction could be enqueued.
 */
bool ltc2485PrepareConfig (Seqio *seqio, int configuration);

/* Prepares a new result readout. The following stop triggers the next conversion.
 */
bool ltc2485PrepareConfigThenReadThenTrigger (Seqio *seqio, int configuration);

/* Prepares a new result readout. The following stop triggers the next conversion.
 */
bool ltc2485PrepareReadThenTrigger (Seqio *seqio);

/** Checks, if LTC2485 readout succeeded and updates the output variable accordingly.
 * Assumes, that a 4 byte read transaction is pending.
 * @param seqio the LTC2485 transaction object.
 * @param v_e24 the place to store the most recently read conversion result in the range +- 1.0 xFS.
 *   FS is 0.5xVref. The result is 25bits in width. In case of overrange, the result is 1<<31 -1 or -1<<31
 *   
 * @return true in case of a valid read out, false otherwise.
 */
bool ltc2485Extract_e24 (Seqio *seqio, Int32 *v_e24);

/** Checks, if LTC2485 readout succeeded and updates the output variable accordingly.
 * Assumes, that a 4 byte read transaction is pending.
 * @param seqio the LTC2485 transaction object.
 * @param vRef_e24 the voltage reference applied to the LTC2485 (3.3V in the most simple case).
 * @param v_e24 the place to store the most recently read conversion result in the range +- 1.0 xFS.
 *   FS is 0.5xVref. The result is 25bits in width. In case of overrange, the result is 1<<31 -1 or -1<<31
 *   
 * @return true in case of a valid read out, false otherwise.
 */
bool ltc2485ExtractV_e24 (Seqio *seqio, Int32 vRef_e24, Int32 *v_e24);

/** Checks, if LTC2485 readout succeeded and updates the output variable accordingly.
 * Assumes, that a 4 byte read transaction is pending.
 * @param seqio the LTC2485 transaction object.
 * @param vRef_e24 the voltage reference applied to the LTC2485 (3.3V in the most simple case).
 * @param k_e10 the place to store the most recently read conversion result.
 * @return true in case of a valid read out, false otherwise.
 */
bool ltc2485ExtractK_e10 (Seqio *seqio, Int32 vRef_e24, Int32 *k_e10);

#endif
