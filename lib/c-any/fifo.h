/*
  fifo.h 
  Copyright 2006-2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __fifo_h
#define __fifo_h

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <integers.h>

/** @file
 * @brief Fundamental partially thread-safe IO-Buffer for buffering, parsing input and formatting output.
 *
 * This implementation of a FIFO is used very successfully in this library mainly due to its partial thread safety.
 * A Fifo can be accessed concurrently from read and write side without further synchronization (Provided size_t vars
 * are accessed by the CPU in one atomic operation). 2 or more concurrent accesses of the read-side or of the write
 * side are not synchronized by Fifo hence require synchronization by the programmer.
 * The major drawback of Fifo is that data layout in memory wraps around after some time. Pointer access to its buffer
 * is therefore only useful before the first wrap-around.
 *
 * The lack of inheritance in C99 prohibits
 * clear separation of read-only and write-only objects, hence Fifo provides both read and write access.
 * As I removed competing data structures line bufferIterator and others, this version of Fifo provides some functions
 * that allow access to contiguous parts of Fifo's memory in some cases.
 * Fifos can be copied by C99 structure assignment. This copying along with Fifo's basic independence of read and
 * write-side allows recursive descend parsers and multi-character lookahead.
 *
 * Naming conventions:
 * Functions, that need checking prior to execution are called fifoReadXXXX, fifoWriteXXXX.
 * Functions that allow optimistic use (with a return value indicating success or failure) are called fifoGetXXXX,
 * fifoPutXXXX.
 * Functions that write/read in network byte order are called fifoPutnetXXXX/fifoGetnetXXXX.
 * Functions that produce human-readable output are called fifoPrintXXXX.
 * Functions that read textual representations of object are called fifoParseXXXX.
 *
 * @todo
 * fifoAppend should be called fifoPutFifo (naming conventions for behaviour).
 * fifoInit should be removed.
 */

/** First in first out character buffer. This buffer is thread safe in the following sense:
 *   1. concurrent read/write status, read and write are allowed.
 *   2. Concurrent accesses of more than one thread at the read side ist NOT allowed.
 *   3. Concurrent accesses of more than one thread at the write side ist NOT allowed.
 */
typedef struct {
	char* 			buffer;
	size_t			size;

	volatile size_t		rTotal;
	volatile size_t		wTotal;

	volatile size_t		rPos;
	volatile size_t		wPos;
} Fifo;

/** This is the same as Fifo, but is used for improved readability.
 */
typedef Fifo ReadFifo;

/** Checks if this Fifo is assigned to a valid value.
 * An definitely invalid Fifo results from Fifo fifo = {} which should be used to create bufferless Fifo variables.
 * Especially useful for checking Fifos afer parsing with functions that return Fifos as result.
 * @param fifo the Fifo to check
 * @return true, if fifo was assigned a value, false otherwise.
 */
static inline bool fifoIsValid(Fifo *fifo) {
	return fifo->size != 0;
}

/** Reset fifo to empty state.
 * WARNING: Not thread safe.
 */
static inline void fifoReset(Fifo *fifo) {
	fifo->rTotal = 0;
	fifo->wTotal = 0;
	fifo->rPos = 0;
	fifo->wPos = 0;
}

/** Initialize a with a buffer of given size for writing (and reading afterwards). The buffer contents is considered
 * invalid data or empty.
 * WARNING: Not thread safe. Do not call if any other functions may be called concurrently
 */
static inline void fifoInitWrite(Fifo *fifo, void *buffer, size_t size) {
	fifoReset(fifo);
	fifo->buffer = (char*)buffer;
	fifo->size = size;
}

/** Initialize a Fifo with a buffer of given size for reading, which means, the whole buffer provided is considered
 * full of valid data.
 * WARNING: Not thread safe. Do not call if any other functions may be called concurrently
 */
static inline void fifoInitRead(Fifo *fifo, void *buffer, size_t size) {
	fifo->buffer = (char*)buffer;
	fifo->size = size;
	fifo->rTotal = 0;
	fifo->wTotal = size;
	fifo->rPos = 0;
	fifo->wPos = size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Checking for bytes (reading)

/** Checks, how many bytes can be read.
 * @return the number of immediately readable bytes. The actual number may be greater at a later time. 
 */
static inline size_t fifoCanRead(Fifo const *fifo) {
	return fifo->wTotal - fifo->rTotal;	// yes, this is correct, despite limited precision.
}

/** Checks, how many bytes can be read from the current position plus an offset.
 * @param fifo the Fifo object
 * @param offset the non-negative offset forwards from the current read position.
 * @return the number of immediately readable bytes at the given offset. The actual number may be greater at a later
 * time. 
 */
static inline size_t fifoCanReadRelative(Fifo const *fifo, size_t offset) {
	const size_t r = fifoCanRead(fifo);
	return r>offset ? r-offset : 0;
}

/** Checks, how many bytes can be read without buffer wraparound. This is important when accessing the Fifo's buffer
 * directly with memcpy.
 * @return the number of immediately readable bytes. The actual number may be greater at a later time. 
 */
size_t fifoCanReadLinear(Fifo const *fifo);

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Checking for bytes (writing)

/** Checks, how many bytes can be written.
 * @return the number of immediately writable bytes. The actual number may be greater at a later time. 
 */
static inline size_t fifoCanWrite(Fifo const *fifo) {
	return fifo->size - fifo->wTotal + fifo->rTotal;	// yes, this is correct.
}

/** Checks, how many bytes can be written without buffer wraparound. This is important when accessing the Fifo's buffer
 * directly with memcpy.
 * @return the number of immediately writable bytes. The actual number may be greater at a later time. 
 */
size_t fifoCanWriteLinear(Fifo const *fifo);


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Reading

/** Reads a single char. Make sure there is a byte available before.
 * @see fifoCanRead()
 */
static inline char fifoRead(Fifo *fifo) {
	const char c = fifo->buffer[fifo->rPos];
	fifo->rTotal++;
	fifo->rPos++; if (fifo->rPos==fifo->size) fifo->rPos = 0;	// short for: rPos = (rPos+1)%size
	return c;
}

/** Reads a block of data. Make sure there are enough bytes in the Fifo before.
 * @see fifoCanRead(), fifoGetN().
 */
static inline void fifoReadN(Fifo *fifo, void *data, size_t n) {
	const size_t n1 = fifo->size - fifo->rPos >= n ? n : fifo->size - fifo->rPos;
	const size_t n2 = n - n1;

	memcpy(data, &fifo->buffer[fifo->rPos], n1);
	memcpy((char*)data + n1, &fifo->buffer[0], n2);
	fifo->rTotal += n;
	fifo->rPos += n1; if (fifo->rPos==fifo->size) fifo->rPos = 0;
	fifo->rPos += n2;
}

/** Looks at the next character to read. Make sure there is a byte available before. This is a cheap function.
 */
static inline char fifoLookAhead(const Fifo *fifo) {
	return fifo->buffer[fifo->rPos];
}

/** Looks at a character forward in the stream.
 * @param fifo the Fifo object
 * @param pos the non-negative offset from the current position. Offset 0 is the next character to read.
 */
static inline char fifoLookAheadRelative(const Fifo *fifo, size_t pos) {
	const size_t index = fifo->rPos+pos;

	return fifo->buffer[ index<fifo->size ? index : index-fifo->size ];
}

/** Extracts a block of bytes without modifying the Fifo itself - kind of a n-bytes lookahead.
 * You have to check first, if this function operates on valid data using fifoCanReadRelative() or similar.
 * @param fifo the Fifo object
 * @param offset the non-negative offset from the current reading position
 * @param data the destination buffer
 * @param n the number of bytes
 */
static inline void fifoPeekRelative(Fifo *fifo, size_t offset, void *data, size_t n) {
	const size_t pos1 = fifo->rPos + offset;	// defeat volatile keyword by copying
	const size_t pos = pos1 < fifo->size ? pos1 : pos1 - fifo->size;
	const size_t n1 = fifo->size - pos >= n ? n : fifo->size - pos;
	const size_t n2 = n - n1;

	memcpy(data, &fifo->buffer[pos], n1);
	memcpy((char*)data + n1, &fifo->buffer[0], n2);
}

/** Copies the read positions of a fifo.
 * @param fifoCopy the destination
 * @param fifo the source
 */
static inline void fifoCopyReadPosition(Fifo *fifoCopy, Fifo *fifo) {
	fifoCopy->rTotal = fifo->rTotal;
	fifoCopy->rPos = fifo->rPos;
}

/** Gets a pointer to the reading position.
 * @param fifo the Fifo object
 */
static inline const char* fifoReadLinear(Fifo const *fifo) {
	return &fifo->buffer[fifo->rPos];
}

/** Skips n input characters.
 */
static inline void fifoSkipRead(Fifo *fifo, size_t n) {
	fifo->rTotal += n;
	fifo->rPos = (fifo->rPos+n) % fifo->size;
}

/** Advances the read position to match a given position.
 * Currently an inefficient (yet reliable) implementation.
 * @param fifo the Fifo object
 * @param readPosition the new reading position in the fifo. This MUST be a position previously written to.
 */
static inline void fifoSkipToRead(Fifo *fifo, size_t readPosition) {
	while (fifo->rPos != readPosition) fifoRead(fifo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Writing

/** Writes a single char. Make sure there is enough free space in the Fifo before.
 * @see fifoCanWrite()
 */
static inline void fifoWrite(Fifo *fifo, char c) {
	fifo->buffer[fifo->wPos] = c;
	fifo->wTotal++;
	fifo->wPos++; if (fifo->wPos==fifo->size) fifo->wPos = 0;	// short for: wPos = (wPos+1)%size
}

/** Write a block of data. Make sure there is enough free space in the Fifo before.
 * @see fifoCanWrite()
 */
static inline void fifoWriteN(Fifo *fifo, const void *data, size_t n) {
	const size_t n1 = fifo->size - fifo->wPos >= n ? n : fifo->size - fifo->wPos;
	const size_t n2 = n - n1;

	memcpy(&fifo->buffer[fifo->wPos], data, n1);
	memcpy(&fifo->buffer[0], (const char*)data + n1, n2);
	fifo->wTotal += n;
	fifo->wPos += n1; if (fifo->wPos==fifo->size) fifo->wPos = 0;
	fifo->wPos += n2;
}

/** Copys the write positions of a fifo.
 * @param fifoCopy the destination
 * @param fifo the source
 */
static inline void fifoCopyWritePosition(Fifo *fifoCopy, Fifo *fifo) {
	fifoCopy->wTotal = fifo->wTotal;
	fifoCopy->wPos = fifo->wPos;
}

/** Gets a pointer to the writing position.
 * If you've written n linear bytes using this pointer, you most probably want to validate these bytes using the
 * function fifoSkipWrite().
 * @see fifoSkipWrite fifoCanReadLinear
 * @param fifo the Fifo object
 */
static inline char* fifoWriteLinear(Fifo const *fifo) {
	return &fifo->buffer[fifo->wPos];
}

/** Skips n output characters, producing a gap of unwritten data.
 */
static inline void fifoSkipWrite(Fifo *fifo, size_t n) {
	fifo->wTotal += n;
	fifo->wPos = (fifo->wPos+n) % fifo->size;
}

/** Append the contents of one fifo to another.
 * @param fifo destination buffer.
 * @param source source buffer
 * @return true, if destination fifo was big enough, false otherwise (and no action taken).
 */
bool fifoPutFifo(Fifo *fifo, Fifo *source);

/** Transfers chars from one fifo to another. The chars are extracted from the source fifo and appended to the
 * destination fifo. Exactly the specified amount of chars is transferred, or no chars are transferred at all.
 * @param fifo destination buffer.
 * @param source source buffer
 * @param n the number of chars to transfer.
 * @return true, if destination fifo was big enough and the source contained enough readable chars; false otherwise
 * (and no action taken).
 */
bool fifoMoveFifo (Fifo *fifo, Fifo *source, size_t n);

// somewhat doubtful function:
/** Invalidates the characters from the current reading position up to the end. No characters are readable
 * until they are validated again. Be careful: this function does not move the write pointer, i.e. does not undo
 * the writes, but simply reduces the readable bytes to 0.
 * WARNING: Not thread safe.
 */
static inline void fifoInvalidateWrites(Fifo *fifo) {
	fifo->wTotal = fifo->rTotal;
}

// somewhat doubtful function:
/** Validates one more character and makes it available. Be careful: this function does not move the write pointer.
 * It merely increases the number of readable bytes. Do use in conjuction with fifoInvalidateWrites ONLY.
 * WARNING: Not thread safe.
 */
static inline void fifoValidateWrite(Fifo *fifo) {
	fifo->wTotal++;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Other functions.

/** Initializes a Fifo for re-writing an already written on. Only the write-side of the rewriter is meaningful.
 * @param rewriter a Fifo object without allocated buffer
 * @param fifo an already written fifo. The current reading position serves as position for re-writing.
 */
static inline void fifoInitRewrite(Fifo *rewriter, Fifo *fifo) {
	rewriter->buffer = fifo->buffer;
	rewriter->size = fifo->size;
	rewriter->wPos = fifo->rPos;
	rewriter->wTotal = rewriter->size - fifoCanRead(fifo);
	rewriter->rPos = fifo->rPos;
	rewriter->rTotal = 0;
}

/** Writes n equal characters.
 */
static inline void fifoFill(Fifo *fifo, char c, size_t n) {
	while (n--) fifoWrite(fifo,c);
}

#if 0
/** Checks, how many bytes of contiguous memory can be read.
 * @return the number of immediately readable bytes. The actual number may be greater at a later time. 
 */
static inline size_t fifoCanReadContiguous(Fifo const *fifo) {
	const size_t maxContiguous = fifo->size - fifo->rPos;
	const size_t readable = fifo->wTotal - fifo->rTotal;	// yes, this is correct, despite limited precision.
	return maxContiguous < readable ? maxContiguous : readable;
}

/** Retrieves a pointer to contiguous reable memory.
 * @param fifo the Fifo object.
 * @return a read-only pointer to the data from current reading position.
 */
static inline const char* fifoReadContiguousLock(Fifo *fifo) {
	return &fifo->buffer[fifo->rPos];
}

/** Releases contiguous memory from the read side.
 * @param fifo the Fifo object.
 * @param n the number of bytes to release (discard!). This must be at most the number returned by
 * fifoCanReadContiguous
 * @see fifoCanReadContiguous
 */
static inline void fifoReadContiguousRelease(Fifo *fifo, size_t n) {
	fifo->rPos += n;
	fifo->rTotal += n;
}
#endif


/** Remove leading chars from buffer.
 * @param fifo character source.
 * @param print a print function disposing off a single character at once. Typically an IO-function. Providing 0 as
 *   function results in the charaters being discarded after reading from the fifo
 * @param nMin the minimum number of characters to be removed.
 * @param nMax the maximum number of characters to be removed.
 * @return true, if fifo contained at least nMin chars, false if not.
 */
bool fifoExtract(Fifo *fifo, void (*print)(char), unsigned nMin, unsigned nMax);


/** Remove all chars from buffer.
 * @param fifo character source.
 * @param print a print function disposing off a single character at once. Typically an IO-function.
 */
void fifoFlush(Fifo *fifo, void (*print)(char));

static inline bool fifoPutN(Fifo *fifo, void const *dest, size_t n) {
	if (n<=fifoCanWrite(fifo)) {
		fifoWriteN(fifo,dest,n);
		return true;
	}
	else return false;
}

static inline bool fifoGetN(Fifo *fifo, void *dest, size_t n) {
	if (n<=fifoCanRead(fifo)) {
		fifoReadN(fifo,dest,n);
		return true;
	}
	else return false;
}

/** Reads a 8-bit integer.
 * @param fifo character source.
 * @param int8 destination buffer.
 * @return true if enough characters were available, false otherwise and Fifo remains unchanged.
 */
static inline bool fifoGetUint8(Fifo *fifo, Uint8 *int8) {
	if (fifoCanRead(fifo)) {
		*int8 = (Uint8) fifoRead(fifo);
		return true;
	}
	else return false;
}

/** Writes a 8-bit integer to a binary stream.
 * @param fifo character source.
 * @param int8 destination buffer.
 * @return true if enough characters were available, false otherwise and Fifo remains unchanged.
 */
static inline bool fifoPutInt8(Fifo *fifo, int int8) {
	if (fifoCanWrite(fifo)) {
		fifoWrite(fifo,(char)int8);
		return true;
	}
	else return false;
}

/** Reads a 16-bit integer.
 * @param fifo character source.
 * @param int16 destination buffer.
 * @return true if enough characters were available, false otherwise and Fifo remains unchanged.
 */
bool fifoGetUint16(Fifo *fifo, Uint16 *int16);

/** Reads a 16-bit integer in big endian order.
 * @param fifo character source.
 * @param int16 destination buffer.
 * @return true if enough characters were available, false otherwise and Fifo remains unchanged.
 */
bool fifoGetnetUint16(Fifo *fifo, Uint16 *int16);

/** Reads a 32-bit integer.
 * @param fifo character source.
 * @param int32 destination buffer.
 * @return true if enough characters were available, false otherwise and Fifo remains unchanged.
 */
bool fifoGetUint32(Fifo *fifo, Uint32 *int32);

/** Reads a 32-bit integer in big endian order.
 * @param fifo character source.
 * @param int32 destination buffer.
 * @return true if enough characters were available, false otherwise and Fifo remains unchanged.
 */
bool fifoGetnetUint32(Fifo *fifo, Uint32 *int32);

/** Writes a 16-bit integer.
 * @param fifo output destination
 * @param int16 the value to write.
 * @return true if the destination was able to take the characters.
 */
bool fifoPutInt16(Fifo *fifo, int int16);

/** Writes a 16-bit integer in big endian order.
 * @param fifo output destination
 * @param int16 the value to write.
 * @return true if the destination was able to take the characters.
 */
bool fifoPutnetInt16(Fifo *fifo, int int16);

/** Writes a 32-bit integer.
 * @param fifo output destination
 * @param int32 the value to write.
 * @return true if the destination was able to take the characters.
 */
bool fifoPutInt32(Fifo *fifo, int int32);

/** Writes a 32-bit integer in big endian order.
 * @param fifo output destination
 * @param int32 the value to write.
 * @return true if the destination was able to take the characters.
 */
bool fifoPutnetInt32(Fifo *fifo, int int32);

/** Writes the contents of a Fifo into a buffer and terminates it with a 0-character. The Fifo is NOT modified.
 * @param fifo the Fifo object
 * @param buffer a memory block to hold the contents of the buffer
 * @param bufferSize the size of the memory block.
 * @return true, if the buffer contents plus a trailing 0 fit into the buffer, false otherwise.
 */
bool fifoToStringZ(Fifo *fifo, char *buffer, size_t bufferSize);

////////////////////////////////////////////////////////////////////////////////////////////////////
// deprecated functions
//

/** Append the contents of one fifo to another.
 * @param fifo destination buffer.
 * @param source source buffer
 * @return true, if destination fifo was big enough, false otherwise
 * @deprecated this function is replaced by fifoPutFifo().
 */
static inline bool fifoAppend(Fifo *fifo, Fifo *source) {
	return fifoPutFifo(fifo,source);
}

/** Initialize a with a buffer of given size.
 * WARNING: Not thread safe. Do not call if any other functions may be called concurrently
 * @deprecated: a better name for this function is fifoInitWrite.
 */
static inline void fifoInit(Fifo *fifo, void *buffer, size_t size) {
	fifoReset(fifo);
	fifo->buffer = (char*)buffer;
	fifo->size = size;
}


#endif

