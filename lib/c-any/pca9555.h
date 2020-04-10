#ifndef __pca9555_h
#define __pca9555_h

/** @file
 * @brief PCA9555 I2C port expander constants.
 * 
 * The PCA9555 has 4 sets of (two) 8-bit registers. These sets are IN,OUT,INV and CONF.
 * Reads and writes are variable in length and if more than one byte are to be read/written, then the registers within
 * the set are toggled on every additional byte. This makes it easy to implement 8-bit transfers or 16-bit transfers.
 * PCA9555 has 3 address input pins for a total of 8 different (consecutive) addresses on the bus.
 *
 * A write transfer is as follows: master sends I2C-address+W, destination register (IN0..CONF1), 1.byte, 2.byte, etc.
 *
 * A read transfer is as follows: master sends I2C-address+W, source register, I2C-address+R, receive 1.byte, 2.bytes,
 * etc. Everything from including 2.byte is optional.
 *
 * The /INT-pin of PCA9555 becomes active, when the input pins change. It becomes inactive on an input read.
 */

enum {
	PCA9555_IN0,		///< input register 0
	PCA9555_IN1,		///< input register 1
	PCA9555_OUT0,		///< output register 0
	PCA9555_OUT1,		///< output register 1
	PCA9555_INV0,		///< input port bit inversion register 0
	PCA9555_INV1,		///< input port bit inversion register 1
	PCA9555_CONF0,		///< direction register 0, 0=output, 1=input
	PCA9555_CONF1,		///< direction register 1, 0=output, 1=input

	PCA9555_ADDRESS=0x40>>1,	///< base address of PCA9555, up to 8 devices on bus: +0..+7
};


#endif

