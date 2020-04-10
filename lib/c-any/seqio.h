/*
  seqio.h 
  Copyright 2011,2015 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __seqio_h
#define __seqio_h

/** @file
 * @brief Sequential IO consisting of multiple data blocks mainly for I2C communications.
 *
 * A Seqio object is a buffer between a user program an a device driver in an asynchronous IO scenario. The user
 * program defines the IO transaction. The device driver extracts the bytes to send from the Seqio write buffer and
 * stores the received bytes in the Seqio read buffer. After IO completion the user program can extract the received
 * bytes from the Seqio read buffer using the functions seqioCanRead() and seqioRead().
 * An IO transaction consists of one or more reads and/or writes, each of which consists of one or more bytes.
 * A sequence of consecutive writes can be joined to a single write unless explicit bounds are included. The same is
 * true for sequences of reads. Any transfer marked with the BOUND flag will not be joined with the preceding transfer.
 * A read transfer can never be joined with a write transfer and vice versa.
 *
 * A hardware driver extracts actions from the transaction object using the seqioAction() function. Actions can be
 * reading or writing or end. First and last bytes and empty transaction are indicated in the action values as bit
 * fields. As an example, one action (1 byte read action) can be: read, first, last. That means, multiple elementary
 * properties may be present in one action.
 * BlockActions (2016) are an alternative to accessing a byte at a time. The BlockAction functions allow to get the
 * next - possibly multi-byte - action in one call and confirm its proper execution later. BlockActions preserve the
 * original sequence of the Seqio build-up.
 *
 * Empty transactions need special explanation. There are two empty transactions: the empty read and the empty write.
 * As an example, both transactions exist in the I2C protocol and consist of addressing and acknowledging without
 * transfer of payload data. Seqio returns the special actions READ_EMPTY and WRITE_EMPTY to the device driver to
 * signal empty transactions. An empty transaction does not return END until the empty transactions are skipped with
 * seqioActionReadEmpty() and seqioActionWriteEmpty() as if they consisted of a special empty character. Empty
 * transactions are encoded with immediate buffers of size 0. Empty transactions embedded in a sequence are not
 * tested, yet (05/2015). Empty transactions are not supported by all I2C devices.
 *
 * Should I include a delay element for devices with a known (answer) processing time ?? (08/2015)
 * 
 * Seqio implements data transfer in both directions in a half duplex mode. At a given time a read transfer can be
 * active or a write transfer but not both.
 * The function seqioPrepareEnd() is the final step before the Seqio object is put into possession of the hardware
 * driver. Once seqioIsDone() or seqioIsJustDone() return true, the Seqio object is 'disowned' by the hardware driver
 * and may be safely manipulated by the user program.
 *
 * When used for I2C communications, it's up to the hardware driver to maintain the I2C address of the slave device.
 * Typically a seqioI2cTransaction() function has the slave address as a parameter and stores it for the time of the
 * transaction (which may require repeated starts!).
 * Seqio targets mainly at 32bit systems (32bit integers and 32bit pointers). 64bit systems will waste a few bytes.
 * This code is limited to little endian systems.
 *
 * seqio.o is about 2kiB of code size.
 */

#include <integers.h>
#include <stdbool.h>
#include <fifo.h>

typedef Int SeqioElement;

/** The encoding of the seqio action sequence: reading/writing immediate or indirect data, explicit bounds.
 */
enum {
	// implemenentation secrets - do not use in user code...
	SEQIO_W_BIT		=0x10000000,	///< all write operations have it, others don't
	SEQIO_R_BIT		=0x20000000,	///< all read operations have it, others don't
	SEQIO_P_BIT		=0x40000000,	///< IMPORTANT: every OP on a buffer has it, all others don't
	SEQIO_B_BIT		=0x80000000,	///< marks a block bound within multiple reads or writes
	SEQIO_L_SHIFT		=24,		///< length position for immediate read/write (1..3)
	SEQIO_L_MASK		=0x00FFFFFF,	///< length for block reads/writes can be large

	// public constants
	SEQIO_END		=0,		///< Use this to terminate a sequence.
	SEQIO_WRITE		=SEQIO_W_BIT | 1<<SEQIO_L_SHIFT,	///< write 1 immediate byte
	SEQIO_WRITE2		=SEQIO_W_BIT | 2<<SEQIO_L_SHIFT,	///< write 2 immediate bytes
	SEQIO_WRITE3		=SEQIO_W_BIT | 3<<SEQIO_L_SHIFT,	///< write 3 immediate bytes
	SEQIO_READ		=SEQIO_R_BIT | 1<<SEQIO_L_SHIFT,	///< driver shall read 1 immediate byte
	SEQIO_READ2		=SEQIO_R_BIT | 2<<SEQIO_L_SHIFT,	///< driver shall read 2 immediate bytes
	SEQIO_READ3		=SEQIO_R_BIT | 3<<SEQIO_L_SHIFT,	///< driver shall read 3 immediate bytes
	SEQIO_WRITE_BUFFER	=SEQIO_W_BIT | SEQIO_P_BIT,		///< write memory block
	SEQIO_READ_BUFFER	=SEQIO_R_BIT | SEQIO_P_BIT,		///< driver shall read memory block
	SEQIO_BOUND		=SEQIO_B_BIT,				///< perform repeated start in I2C
};

typedef enum {
	SEQIO_PREPARE = 0,	///< sequence is being prepared
	SEQIO_ACTION,		///< sequence prepared and now ready for being processed by driver
	SEQIO_PENDING,		///< driver has not yet confirmed all bytes sent
	SEQIO_DONE,		///< driver finished with sequence successfully
	SEQIO_DONE2,		///< driver finished with sequence successfully, seqioIsJustDone() no longer true
	SEQIO_ERROR,		///< driver finished with sequence with error
	SEQIO_ERROR2,		///< driver finished with sequence with error, seqioIsJustDone() no longer true
} SeqioStatus;

typedef struct {
	SeqioElement*	elements;	///< IO sequence
	Uint16		size;		///< byte size of the elements array
	Uint16		position;	///< element position
	Uint16 volatile	status;		///< See SeqioStatus
	Uint16		charPosition;	///< character position while processing
} Seqio;

/** Initializes a Seqio object.
 * @param seqio the sequential IO object.
 * @param elements a (static) array used as buffer area
 * @param size the size (bytes) of the buffer array.
 */
void seqioInit(Seqio *seqio, SeqioElement* elements, int size);

/** Resets Seqio for preparation.
 * @param seqio the sequential IO object.
 */
void seqioReset(Seqio *seqio);

// preparing: interface of user code

/** Writes 'zero' immediate bytes thus creating the empty write transaction.
 * @param seqio the sequential IO object.
 * @return true if successful, false if buffer overflow or parameter error.
 */
bool seqioPrepareWriteEmpty (Seqio *seqio);

/** Writes 1 to 3 immediate bytes.
 * @param seqio the sequential IO object.
 * @param cs the data to write in little endian order.
 * @param n a value between 1 and 3.
 * @param bound true to mark this write as a block, false to (potentially) merge the write with the one before.
 * @return true if successful, false if buffer overflow or parameter error.
 */
bool seqioPrepareWrite(Seqio *seqio, int cs, int n, bool bound);

/** Writes data from a buffer.
 * @param seqio the sequential IO object.
 * @param data the pointer to the write data. Write data must exist until the IO transaction is finished.
 * @param n data size. Zero bytes are NOT allowed.
 * @param bound true to mark this write as a block, false to (potentially) merge the write with the one before.
 */
bool seqioPrepareWriteBlock(Seqio *seqio, const void *data, int n, bool bound);

/** Reads 'zero' immediate bytes thus creating the empty read transaction.
 * @param seqio the sequential IO object.
 * @return true if successful, false if buffer overflow or parameter error.
 */
bool seqioPrepareReadEmpty (Seqio *seqio);

/** Reads 1 to 3 bytes.
 * @param seqio the sequential IO object.
 * @param n the number of butes to read (later), range = 1..3. n=0 is allowed for the first and only
 *   element as an exception.
 * @param bound true to mark this read as a block, false to (potentially) merge the read with the one before.
 */
bool seqioPrepareRead(Seqio *seqio, int n, bool bound);

/** Reads 1 to n bytes.
 * @param seqio the sequential IO object.
 * @param data the buffer for storing the bytes to read.
 * @param n the number of bytes to read (later). Zero bytes are NOT allowed.
 * @param bound true to mark this read as a block, false to (potentially) merge the read with the one before.
 */
bool seqioPrepareReadBlock(Seqio *seqio, void *data, int n, bool bound);

/** Marks the current position as end of IO. The state transition to SEQIO_ACTION and a device driver may access the
 * IO objects from this point.
 * @param seqio the sequential IO object.
 * @return false if the IO object was not in state SEQIO_PREPARE.
 */
bool seqioPrepareEnd(Seqio *seqio);

/** Prepares an IO object for replay. It must be prepared successfully before.
 */
bool seqioPrepareReplay(Seqio *seqio);

/** Checks, if the device driver has finished the transaction. Can be called at any time.
 * @param seqio the sequential IO object.
 * @return true, if user code can access the IO object for reading/error checking.
 */
bool seqioIsDone(const Seqio *seqio);

/** Checks, if the device driver has finished the transaction, exactly how seqioIsDone() does.
 * However, this function returns true only once - it performs edge detection. The similar function seqioIsDone() will
 * still return true - it is not affected.
 * @param seqio the sequential IO object.
 * @return true, if user code can access the IO object for reading/error checking.
 */
bool seqioIsJustDone(Seqio *seqio);

/** Checks for success after the IO is completed. Can be called at any time - you don't have to check for completion
 * before.
 * @param seqio the sequential IO object.
 * @return true if all bytes have been transmitted/received or false if anything went wrong.
 */
bool seqioIsSuccessful(const Seqio *seqio);

// reading of acquired data: interface of user code
/** Checks if more bytes are available for reading. Must not be called until a device driver finished successfully
 * with this IO object.
 * @param seqio the sequential IO object.
 * @return true if at least one more byte can be read.
 */
bool seqioCanRead(Seqio *seqio);

/** Reads one byte. Must not be called unless seqioCanRead() returns true (believe this!!).
 * This function adjusts the read pointer, so don't omit it, even if you're sure bytes are available.
 * @param seqio the IO transaction object
 */
char seqioRead(Seqio *seqio);

////////////////////////////////////////////////////////////////////////////////////////////////////
// processing: interface to IO driver

/** Defines what a device driver is supposed to do next.
 * The following constants are bits that need to be ORed together to form suitable action descriptions. This means
 * that if-else conditions are more suitable than switch statements as more than one combination may occur.
 *
 * An empty transaction (read or write) is allowed, but may consist only of one SeqioElement. An empty transaction
 * has neither a first byte nor a last byte, but is indicated by the 'empty' flag.
 */
typedef enum {
	SEQIO_ACTION_READ	=0x01,	///< read action, non-empty
	SEQIO_ACTION_WRITE	=0x02,	///< write action, non-empty
	SEQIO_ACTION_FIRST_BYTE	=0x04,	///< first byte of a read/write action
	SEQIO_ACTION_LAST_BYTE	=0x08,	///< last byte of a read/write action
	SEQIO_ACTION_READ_EMPTY	=0x10,	///< empty read transaction
	SEQIO_ACTION_WRITE_EMPTY=0x20,	///< empty write transaction
	SEQIO_ACTION_END	=0x80,	///< completed - either with or without success
} SeqioAction;
////////////////////////////////////////////////////////////////////////////////////////////////////
// block oriented functions.

typedef struct {
	Uint32		action;
	union {
		void		*data;
		const void	*roData;	// just for convenience of use in a driver.
	};
	Uint32		n;
} SeqioBlockAction;

/** Checks, if zero, one or more blocks are available. This is important for I2C drivers to decide about a stop
 * condition in advance.
 * @param seqio the sequential IO object.
 * @return 0 if no block is available, 1 if one final block is available, 2 if AT LEAST 2 blocks are available (even if
 *   these are empty blocks),
 */
Uint32 seqioBlockActionEstimate (const Seqio *seqio);

SeqioBlockAction seqioBlockAction (const Seqio *seqio);

void seqioBlockActionConfirm (Seqio *seqio);

/** Checks, what the hardware driver should do next.
 * @param seqio the sequential IO object.
 * @return a bit mask indicating read/write/end and first/last/empty byte
 */
SeqioAction seqioAction(const Seqio *seqio);

/** Reads a single character from the IO object buffer. This character must be transmitted by the hardware.
 * Call seqioActionConfirm() after the character is processed.
 * @param seqio the sequential IO object.
 */
char seqioActionRead(Seqio *seqio);

/** Confirms, that the last byte read from Seqio is successfully sent by the hardware. This function triggers the
 * transition from SEQIO_PENDING to SEQIO_END. It must be called only in SEQIO_PENDING state.
 * @param seqio the sequential IO object.
 */
void seqioActionConfirm(Seqio *seqio);

/** Reads zero characters from the IO object buffer. This ends the empty transaction.
 * Must be called in response to an empty read transaction action code, only.
 * @param seqio the sequential IO object.
 */
void seqioActionReadEmpty (Seqio *seqio);

/** Writes a single character to the IO object buffer. This character was received by the hardware and will be
 * read by the user program from the IO object buffer.
 * @param seqio the sequential IO object.
 * @param c the character to write
 */
void seqioActionWrite(Seqio *seqio, char c);

/** Writes zero characters to the IO object buffer. This ends the empty transaction
 * Must be called in response to an empty write transaction action code, only.
 * @param seqio the sequential IO object.
 */
void seqioActionWriteEmpty (Seqio *seqio);

/** Indicates abnormal termination of the IO operation when called by the device driver.
 * @param seqio the sequential IO object.
 */
void seqioActionError(Seqio *seqio);

/** Prints in human-readable format.
 * @param fifo output destination
 * @param status the status of a sequential IO object.
 * @return true in case successfully written, false otherwise.
 */
bool fifoPrintSeqioStatus (Fifo *fifo, int status);

/** Prints in human-readable format.
 * @param fifo output destination
 * @param seqio the sequential IO object.
 * @return true in case successfully written, false otherwise.
 */
bool fifoPrintSeqio (Fifo *fifo, const Seqio *seqio);

#endif

