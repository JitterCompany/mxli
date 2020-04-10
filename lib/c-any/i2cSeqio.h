#ifndef __i2cSeqio_h
#define __i2cSeqio_h

/** @file
 * @brief Extends Seqio to an addressed Seqio - which is the appropriate unit for I2C transations.
 */

#include <seqio.h>

/** A Seqio object specialized for I2C.
 */
typedef struct {
	Seqio	seqio;		///< LM73 I2C transaction object.
	int	address;	///< address the 7-bit I2C-address, right centered.
} I2cSeqio;

#endif

