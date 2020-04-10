/*
  sharedConfig.h - Shared program/driver configuration parameters.
  Copyright 2015 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef sharedConfig_h
#define sharedConfig_h
/** @file
 * @brief Shared program/driver configuration parameters.
 *
 * The idea behind this module is to have an easy and centralized way of defining program/driver constants easily
 * and space-saving.
 *
 * As an example, all drivers accessing an I2C bus should have some access timings in common, whereas each device
 * has its own special timings in addition. It would be bad practice to put constants into the library of drivers
 * thus rendering the I2C bus timings of the library drivers practically unconfigurable (source change required).
 * The approach implemented here uses small offsets (small pointers) into a (const) global configuration area to store
 * configuration parameters. Additionally different constant sizes may be used to compress the configuration area.
 *
 * The (default) implementation of the configuration read function sharedConfig8Read() relies on the structure
 * sharedConfig8 to be defined and initialized. However, you are free to provide your own implementation of
 * sharedConfig8Read() with a different storage model. Anyway, I insist on a configuration to be constant at runtime.
 *
 * Shared config is fast.
 */

#include <integers.h>
#include <stdbool.h>
#include <attribute.h>

enum {
	_SHARED_CONFIG8_TYPE	=6,
	SHARED_CONFIG8_LONG_	=0,
	SHARED_CONFIG8_SHORT_	=1,
	SHARED_CONFIG8_BYTE_	=2,
	SHARED_CONFIG8_BOOL_	=3,
	SHARED_CONFIG8_LONG	=SHARED_CONFIG8_LONG_<<_SHARED_CONFIG8_TYPE,
	SHARED_CONFIG8_SHORT	=SHARED_CONFIG8_SHORT_<<_SHARED_CONFIG8_TYPE,
	SHARED_CONFIG8_BYTE	=SHARED_CONFIG8_BYTE_<<_SHARED_CONFIG8_TYPE,
	SHARED_CONFIG8_BOOL	=SHARED_CONFIG8_BOOL_<<_SHARED_CONFIG8_TYPE,
	SHARED_CONFIG8_OFFSET	=(1<<_SHARED_CONFIG8_TYPE)-1,
	// range 0..0x3F	= 64 locations
};

typedef struct {
	const void* const	values;
	const Uint16		size;		///< size in bytes
} SharedConfig8;

extern SharedConfig8 sharedConfig8;	///< this is where you should place the configuration

/** Reads a (constant) configuration parameter. Please note, that small integers should be placed at low addresses.
 * Large integers have a further reach in the very limited address space. Index 0 for a 32-bit value consumes addresses
 * 0..31 for booleans, 0..3 for bytes, 0..1 for shorts.
 * @param what the ID of the configuration parameter which consists of a size specifier SHARED_CONFIG_LONG, ...
 *   and an index (small number).
 * @return the configuration value, extended to 32bits.
 */
Uint32 sharedConfig8Read (Uint8 what) CONST_FUNCTION;

#endif

