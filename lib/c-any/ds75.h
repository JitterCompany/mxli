/*
  ds75.h 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef ds75_h
#define ds75_h

/** @file
 * @brief Generic DS75 functionality on top of seqI2c.
 *
 * One Seqio object per DS75 must be provided by the user. Communication should be performed asynchronously using the
 * module seqio by the user. This generic module cannot perform the actual (hardware-dependend) IO. Currently only
 * continuous temperature acquisition is supported.
 */

#include <seqio.h>

enum Ds75ConfigurationBits {
	_DS75_CONFIG_SD		=0,	///< shutdown: 0=running, 1=shutdown
	_DS75_CONFIG_TM		=1,	///< thermostat mode: 0=comparator, 1=interrupt
	_DS75_CONFIG_POL	=2,	///< output polarity: 0=active low, 1=active high
	_DS75_CONFIG_F		=3,	///< 2 bits fault tolerance for out-of-limit to trigger.
		DS75_CONFIG_F1_	=0,	///< 1 out-of-limit for trigger
		DS75_CONFIG_F2_	=0,	///< 2 out-of-limit for trigger
		DS75_CONFIG_F4_	=0,	///< 4 out-of-limit for trigger
		DS75_CONFIG_F6_	=0,	///< 6 out-of-limit for trigger
	DS75_CONFIG_RES_	=5,	///< 2 bits, resolution configuration
	DS75_CONFIG_RES_05_,		///< resolution: 0.5 C, 25ms
	DS75_CONFIG_RES_025_,		///< resolution: 0.25 C, 50ms 
	DS75_CONFIG_RES_0125_,		///< resolution: 0.125 C, 100ms 
	DS75_CONFIG_RES_00625_,		///< resolution: 0.0625 C, 200ms
};


/** The pointer is transmitted as second byte (after I2C address) in a write.
 */
enum Ds75PointerBits {
	DS75_POINTER_TEMP	=0,
	DS75_POINTER_CONFIG	=1,
	DS75_POINTER_THYST	=2,
	DS75_POINTER_TOS	=3,
};

enum Ds75I2cAddresses {	// binary 1001 A2 A1 A0 = 0x48+A, up-to 8 addresses by hardware.
	DS75_ADDRESS_0	=0x48,		///< all zeros
};

enum Ds75CommandBits {
	DS75_COMMAND_POR	=0x54,	///< send this after address and DS75 resets
};

/** Blocks until the given transaction is completed or has failed.
 * @param seqio the transaction object used for a given DS75.
 * @return true for successfull completion, false for failure.
 */
bool ds75Sync(Seqio *seqio);

/** Configures DS75 for the given temperature resolution.
 * @param seqio the transaction object used for this DS75. Must be 5*4 bytes in buffer size at least.
 * @param configuration the configuration parameters.
 * @return true, if the transaction could be enqueued.
 */
bool ds75Init(Seqio *seqio, int configuration);

/** Checks, if DS75 temperature readout succeeded and updates the temperature variable accordingly.
 * @param seqio the DS75 transaction object. This is reused for issuing further temperature readouts.
 * @param temperatureF1024 the place to store the most recently read temperature.
 * @return true in case of a temperature update, false otherwise.
 */
bool ds75TemperaturePoll(Seqio *seqio, int *temperatureF1024);

/* Prepares a new temperature readout.
 */
bool ds75TemperatureQuery(Seqio *iseqio);

#endif
