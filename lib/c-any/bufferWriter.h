#ifndef bufferWriter_h
#define bufferWriter_h

/** @file
 * @brief Simple writer for contiguous memory - replaces former BufferIterator.
 *
 * This extremely slim implementation is intended for simple tasks where a Fifo is too expensive.
 * Clear separation between reading and writing - please note the slight naming anomaly: The object type is
 * BufferWriter but the function is called bufferCanWrite, not bufferWriterCanWrite.
 */

#include <stddef.h>

typedef struct {
	char*		buffer;		///< start of buffer
	size_t		size;		///< total size of buffer in bytes
	size_t		position;	///< current position of writing
} BufferWriter;	///< Contiguous memory writer.

/** Initializes a new buffer writer.
 * @param writer the BufferWriter object.
 * @param buffer the memory location to be writter.
 * @param size the size of the memory location.
 */
inline static void bufferWriterInit(BufferWriter *writer, void *buffer, size_t size) {
	writer->buffer = (char*)buffer;
	writer->size = size;
	writer->position = 0;
}

/** Checks if more bytes can be written.
 * @param writer the writer object.
 * @return the number of bytes that can be written.
 */
inline static size_t bufferCanWrite(BufferWriter const* writer) {
	return writer->size - writer->position;
}

/** Writes one byte to memory.
 * @param writer the writer object.
 * @param c the byte to write.
 */
inline static void bufferWrite(BufferWriter *writer, char c) {
	writer->buffer [writer->position++] = c;
}

/** Writes up to 4 bytes to memory. Little endian order is applied.
 * @param writer the writer object.
 * @param data the bytes to write.
 * @param n the number of bytes: 0..4.
 */
inline static void bufferWrite4(BufferWriter *writer, unsigned data, size_t n) {
	for (unsigned b=0; b<n; ++b) bufferWrite(writer, (char)(data>>8*b));
}


/** Writes n bytes to memory.
 * @param writer the writer object.
 * @param data the bytes to write.
 * @param n the number of bytes: 0..4.
 */
void bufferWriteN(BufferWriter *writer, const void *data, size_t n);

#endif
