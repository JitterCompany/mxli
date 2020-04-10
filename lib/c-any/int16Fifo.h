/*
  int16Fifo.h 
  Copyright 2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __int16Fifo_h
#define __int16Fifo_h

#include <stdbool.h>
#include <stddef.h>
#include <integers.h>

/** @file
 * @brief Fundamental partially thread-safe Buffer for 16-bit signed values.
 *
 * This implementation of a FIFO is used very successfully in this library mainly due to its partial thread safety.
 * A Fifo can be accessed concurrently from read and write side without further synchronization (Provided size_t vars
 * are accessed by the CPU in one atomic operation). 2 or more concurrent accesses of the read-side or of the write
 * side are not synchronized by Fifo hence require synchronization by the programmer.
 * The major drawback of Fifo is that data layout in memory wraps around after some time. Pointer access to its buffer
 * it therefore only usefull before the first wrap-around.
 *
 * The lack of inheritance in C99 prohibits
 * clear separation of read-only and write-only objects, hence Fifo provides both read and write access.
 * As I removed competing data structures line bufferIterator and others, this version of Fifo provides some functions
 * that allow access to contiguous parts of Fifo's memory in some cases.
 * Fifos can be copied by C99 structure assignment. This copying along with Fifo's basic independence of read and
 * write-side allows recursive descend parsers and multi-character lookahead.
 *
 */

/** First in first out Int16 buffer. This buffer is thread safe in the following sense:
 *   1. concurrent read/write status, read and write are allowed.
 *   2. Concurrent accesses of more than one thread at the read side ist NOT allowed.
 *   3. Concurrent accesses of more than one thread at the write side ist NOT allowed.
 */
typedef struct {
	Int16* 			buffer;
	size_t			size;		///< size in bytes.

	volatile size_t		rTotal;
	volatile size_t		wTotal;

	volatile size_t		rPos;
	volatile size_t		wPos;
} Int16Fifo;

/** Checks if this Int16Fifo is assigned to a valid value.
 * An definitely invalid Int16Fifo results from Int16Fifo fifo = {} which should be used to create bufferless Int16Fifo variables.
 * Especially useful for checking Int16Fifos afer parsing with functions that return Int16Fifos as result.
 * @param fifo the Int16Fifo to check
 * @return true, if fifo was assigned a value, false otherwise.
 */
static inline bool int16FifoIsValid(Int16Fifo *fifo) {
	return fifo->size != 0;
}

/** Reset fifo to empty state.
 * WARNING: Not thread safe.
 */
static inline void int16FifoReset(Int16Fifo *fifo) {
	fifo->rTotal = 0;
	fifo->wTotal = 0;
	fifo->rPos = 0;
	fifo->wPos = 0;
}

/** Initialize a with a buffer of given size for writing (and reading afterwards). The buffer contents is considered
 * invalid data or empty.
 * WARNING: Not thread safe. Do not call if any other functions may be called concurrently
 */
static inline void int16FifoInitWrite(Int16Fifo *fifo, void *buffer, size_t size) {
	int16FifoReset(fifo);
	fifo->buffer = (Int16*)buffer;
	fifo->size = size;
}

/** Initialize a with a buffer of given size for reading, which means, the whole buffer provided is considered full
 * of valid data.
 * WARNING: Not thread safe. Do not call if any other functions may be called concurrently
 */
static inline void int16FifoInitRead(Int16Fifo *fifo, void *buffer, size_t size) {
	fifo->buffer = (Int16*)buffer;
	fifo->size = size;
	fifo->rTotal = 0;
	fifo->wTotal = size/2;
	fifo->rPos = 0;
	fifo->wPos = size/2;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Checking for bytes (reading)

/** Checks, how many Int16 can be read.
 * @return the number of immediately readable values. The actual number may be greater at a later time. 
 */
static inline size_t int16FifoCanRead(Int16Fifo const *fifo) {
	return fifo->wTotal - fifo->rTotal;	// yes, this is correct, despite limited precision.
}

/** Checks, how many bytes can be read from the current position plus an offset.
 * @param fifo the Int16Fifo object
 * @param offset the non-negative offset forwards from the current read position.
 * @return the number of immediately readable bytes at the given offset. The actual number may be greater at a later
 * time. 
 */
static inline size_t int16FifoCanReadRelative(Int16Fifo const *fifo, size_t offset) {
	const size_t r = int16FifoCanRead(fifo);
	return r>offset ? r-offset : 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Checking for bytes (writing)

/** Checks, how many bytes can be written.
 * @return the number of immediately writable bytes. The actual number may be greater at a later time. 
 */
static inline size_t int16FifoCanWrite(Int16Fifo const *fifo) {
	return fifo->size/2 - fifo->wTotal + fifo->rTotal;	// yes, this is correct.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Reading

/** Reads a single value. Make sure there is a byte available before.
 * @see fifoCanRead()
 */
static inline Int16 int16FifoRead(Int16Fifo *fifo) {
	const  Int16 i = fifo->buffer[fifo->rPos];
	fifo->rTotal++;
	fifo->rPos++; if (fifo->rPos==fifo->size/2) fifo->rPos = 0;	// short for: rPos = (rPos+1)%size
	return i;
}

/** Looks at the next value to read. Make sure there is a byte available before. This is a cheap function.
 */
static inline Int16 int16FifoLookAhead(const Int16Fifo *fifo) {
	return fifo->buffer[fifo->rPos];
}

/** Looks at a value forward in the stream.
 * @param fifo the Int16Fifo object
 * @param pos the non-zero offset from the current position.
 */
static inline Int16 int16FifoLookAheadRelative(const Int16Fifo *fifo, size_t pos) {
	const size_t index = fifo->rPos+pos;

	return fifo->buffer[ index<fifo->size/2 ? index : index-fifo->size/2 ];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Writing

/** Writes a single value. Make sure there is enough free space in the Int16Fifo before.
 * @see fifoCanWrite()
 */
static inline void int16FifoWrite(Int16Fifo *fifo, Int16 i) {
	fifo->buffer[fifo->wPos] = i;
	fifo->wTotal++;
	fifo->wPos++; if (fifo->wPos==fifo->size/2) fifo->wPos = 0;	// short for: wPos = (wPos+1)%size
}

#endif

