/*
  dogDriver.h 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef dogDriver_h
#define dogDriver_h

#include <stdbool.h>

/** @file
 * @brief Experimental interface for driving EA-DOGM128 display using a callback function.
 *
 * The addressed kind of displays uses a non-pure SPI interface for control.
 * An additional address line distinguishes command and data bytes. Unlike many other devices which latch data upon
 * de-assertion of the chip select, the display devices used here interpret data as soon as it is it clocked in.
 * I assume that an interrupt service routine handles the transfer of bytes and this interrupt routine uses callbacks
 * to query the bytes to send next. Driving displays is transferring a fixed amount of data bytes, possibly prefixed by
 * some control bytes. The interrupt routine implements most of the iteration, the callback function should remain as
 * simple as possible.
 *
 * A more generic approch would extract the basic functions (address selection, strobing) and provide these as function
 * pointer parameters, maybe.
 */

/** Return codes of the callback function. Additional data is ORed into the lower 8 bits.
 */
typedef enum {
	DOG_CMD		= 0x100,
	DOG_DATA	= 0x200,
	DOG_PREFIX	= 0x400,	// do not increment, progress is made in sub-index.
	DOG_UNDEFINED	= 0,
} DogByte;

/** Data-generating callback function.
 * @param data the data buffer origin.
 * @param index the current index of the byte to send next.
 * @param n the total number of bytes to send
 * @param callback the function that performs the transfer and also the identifier for 'such a transfer' .
 * @param subIndex an index that can be incremented instead of index, to easily implement prefixing of bytes.
 *   subIndex is reset to 0 (LSB 16 bits) if sub-indexing is not used.
 */
typedef DogByte DogDriverCallback(const char *data, int index, int n, int subIndex);

typedef struct {
	DogDriverCallback*	callback;
	int			subIndex;
	const char*		data;
	int			n;
} DogDriverElement;

/** Needed to install weak dummy functions for the dog driver interface.
 * @param elements an array for storing a queue of transaction.
 * @param size the size (bytes) of the elements array. This must be sizeof(DogDriverElement)*2^p in order to get
 * an efficient implementation even on processors without a division instruction.
 */
void dogDriverInit(DogDriverElement *elements, int size);


int dogDriverCanWrite(void);

/** Writes any number of data bytes.
 * @param data an optional pointer to some data.
 * @param n size of optional data
 * @param callback a character-generating function
 * @param subIndex an additional, zero-based index for enumerating prefixes.
 * @return a transcation number (strictly ascending).
 */
int dogDriverTransfer(const void *data, int n, DogDriverCallback *callback, int subIndex);

/** Writes any number of data bytes, if no such transfer is already enqueued and waiting for execution. If a
 * transaction of the same kind - identified by the callback function address - is still awaiting execution then no
 * new transaction is enqueued, but the transaction ID of the previous one is returned.
 * @param data an optional pointer to some data.
 * @param n size of optional data
 * @param callback the function that performs the transfer and also the identifier for 'such a transfer' .
 * @param subIndex an additional, zero-based index for enumerating prefixes.
 * @return the transcation number of the newly created transfer or the transaction number of an equivalent transfer.
 */
int dogDriverTransferNew(const void *data, int n, DogDriverCallback *callback, int subIndex);

/** Returns the number of the latest completed transaction.
 * @param tid the transaction number to check for.
 * @return true if the given transaction is finished.
 */
bool dogDriverTransferCompleted(int tid);

/** Checks, if another byte can be read by the device driver.
 */
bool dogDriverCanRead(void);

/** Reads the next byte.
 * @return the next byte - make sure it exists BEFORE!
 */
DogByte dogDriverReadByte(void);

bool dogDriverTransferCompleted(int tid);

/** Do busy wait until transfer completed.
 */
void dogDriverWaitFor(int tid);

/** Checks, if the dogDriver is still active in background.
 */
bool dogDriverIsActive(void);

/** Must be called by the ISR handling SPI when it goes into inactive state (last byte transmitted, no more
 * interrupts). It is the responsibility of the user code to awake it on the next transfer.
 */
void dogDriverSleep(void);

/** Must be called by user code BEFORE the ISR is going to be awakened.
 */
void dogDriverWillBeActivated(void);


////////////////////////////////////////////////////////////////////////////////////////////////////
// DOG helpers

/** This is an interface function to be implemented in a hardware driver.
 * @return the transfer ID or 0 in case of failure.
 */
int dogDriverStartTransfer(const void *data, int n, DogDriverCallback *callback, int subIndex);


/** Callback function for a block of all-command bytes.
 */
DogByte dogDriverCallbackWriteCmd(const char *data, int index, int n, int subIndex);

/** Callback function for a block of all-data bytes.
 */
DogByte dogDriverCallbackWriteData(const char *data, int index, int n, int subIndex);

int dogDriverWriteCommand(const char* data, int n);

int dogDriverWriteData(const char* data, int n);


#endif
