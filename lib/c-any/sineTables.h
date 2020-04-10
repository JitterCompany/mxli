#ifndef __sineTable_h
#define __sineTable_h

/** @file
 * @brief provides various integer sine tables.
 *
 * Note that the tables are signed and span 255 or 65535 values only.
 */

#include <integers.h>

/** A table that maps 8-bit index to signed +-127 values. Note, that the value -128 is NOT used.
 * Size is 0.25kiB.
 */
extern const Int8 sine8To8[256];

/** A table that maps 8-bit index to signed +-32767 values. Note that the value -32768 is NOT used.
 * Size is 0.5kiB.
 */
extern const Int16 sine8To16[256];

/** A table that maps 10-bit index to signed +-32767 values. Note that the value -32768 is NOT used.
 * Size is 2kiB.
 */
extern const Int16 sine10To16[1024];

/** A table that maps 12-bit index to signed +-32767 values. Note that the value -32768 is NOT used.
 * Size is 8kiB.
 */
extern const Int16 sine12To16[4096];

/** Scale down signed table values. Always round towards 0. Arithmetic shift rounds towards negative infinity which
 * would lead to asymmetric extreme values (like -16..+15).
 * @param value the value to scale down
 * @param bits the number of bits to remove.
 * @result value integer divided by 2^bits, rounded towards 0.
 */
static inline Int32 sineScaleDown(Int32 value, int bits) {
	return value>=0 ? value>>bits : -(-value>>bits);
}

#endif
