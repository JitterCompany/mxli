/*
  fram.h 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef fram_h
#define fram_h

/** @file
 * @brief RAMTRON ferroelectric RAMs common functionality.
 */

#include <fifoPrint.h>

typedef union {
	struct {
		int	revision:3;		///< chip revision
		int	variation:5;		///< bit[4] indicates serial number availability
		int	density:4;		///< size = 16kB ^ density
		int	manufacturer:12;	///< 4 = RAMTRON
	};
	int	int32;
} FramId;

/** Calculates the FRAM size (bytes) from its ID.
 * @return the size of the FRAM in bytes.
 */
static inline int framSizeKib(FramId id) {
	return 1<<(13+id.density);
}

/** Prints FRAM basic params in a human-readable way.
 * @param output the output destination
 * @param id the FRAM ID read from the device (3 bytes).
 * @return true in case of success, false if output exhausted.
 */
bool fifoPrintFramId(Fifo *output, FramId id);

#endif
