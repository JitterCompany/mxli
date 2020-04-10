/*
  int32Cyclic.h - 32bit math that uses 32bit instructions on 32bit CPUs.
  Copyright 2015 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __int32Cyclic_h
#define __int32Cyclic_h

/** @file
 * @brief Cyclic functions using 32bit integer instructions only.
 */

#include <stdbool.h>
#include <integers.h>

/** Compares values that wrap around on overflow (like hardware timers do as an example).
 * @param a the first value
 * @param b the second value
 * @return true if a is less than b (i.e. a will become b, if incremented by less than half the integer range.
 *   The result is correct, even if a or b are close to the wraparound. The result is still correct, if you 
 *   supply signed integers.
 */
static inline bool uint32CyclicLessThan (Uint32 a, Uint32 b) {
	return (Int32)(a-b) < 0;
}

/** Compares values that wrap around on overflow (like hardware timers do as an example).
 * @param a the first value
 * @param b the second value
 * @return true if a is greater than b (i.e. b will become a, if incremented by less than half the integer range.
 *   The result is correct, even if a or b are close to the wraparound. The result is still correct, if you 
 *   supply signed integers.
 */
static inline bool uint32CyclicGreaterThan (Uint32 a, Uint32 b) {
	return (Int32)(a-b) > 0;
}

/** Compares values that wrap around on overflow (like hardware timers do as an example).
 * @param a the first value
 * @param b the second value
 * @return true if a is less or equal than b (i.e. a will become b, if incremented by less than half the integer range.
 *   The result is correct, even if a or b are close to the wraparound. The result is still correct, if you 
 *   supply signed integers.
 */
static inline bool uint32CyclicLessOrEqual (Uint32 a, Uint32 b) {
	return (Int32)(a-b) <= 0;
}

/** Compares values that wrap around on overflow (like hardware timers do as an example).
 * @param a the first value
 * @param b the second value
 * @return true if a is greater than b (i.e. b will become a, if incremented by less than half the integer range.
 *   The result is correct, even if a or b are close to the wraparound. The result is still correct, if you 
 *   supply signed integers.
 */
static inline bool uint32CyclicGreaterOrEqual (Uint32 a, Uint32 b) {
	return (Int32)(a-b) >= 0;
}

#endif
