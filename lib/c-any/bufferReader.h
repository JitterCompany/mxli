#ifndef bufferReader_h
#define bufferReader_h

/** @file
 * @brief Simple reader for contiguous memory - replaces former BufferIterator.
 *
 * This extremely slim implementation is intended for simple tasks where a Fifo is too expensive.
 * Clear separation between reading and writing - please note the slight naming anomaly: The object type is
 * BufferReader but the function is called bufferCanRead, not bufferReaderCanRead.
 */

#include <stddef.h>

typedef struct {
	const char*	buffer;		///< start of buffer
	size_t		size;		///< total size of buffer (contents)
	size_t		position;	///< current position of reading
} BufferReader;	///< Contiguous memory reader of const memory.

/** Initializes a buffer reader.
 * @param reader the BufferReader object.
 * @param buffer the starting address of the memory location to read.
 * @param size the size of the memory location to read.
 */
inline static void bufferReaderInit(BufferReader *reader, const void *buffer, size_t size) {
	reader->buffer = (const char*)buffer;
	reader->size = size;
	reader->position = 0;
}

/** Checks, if more bytes can be read.
 * @param reader the BufferReader object.
 * @return the number of bytes that can be read.
 */
inline static size_t bufferCanRead(BufferReader const* reader) {
	return reader->size - reader->position;
}

/** Reads one byte from memory.
 * @param reader the BufferReader object.
 * @return the next byte from memory.
 */
inline static char bufferRead(BufferReader *reader) {
	return reader->buffer [reader->position++];
}

/** Reads up to 4 bytes from memory.
 * @param reader the BufferReader object.
 * @param n the number of bytes to read: 0..4 .
 * @return the next bytes from memory in an little endian aligned integer. Unread portions are cleared in the result.
 */
inline static unsigned bufferRead4(BufferReader *reader, size_t n) {
	unsigned result = 0;
	for (unsigned b=0; b<n; ++b) result |= (0xff & (unsigned)bufferRead(reader)) << 8*b;
	return result;
}

#endif
