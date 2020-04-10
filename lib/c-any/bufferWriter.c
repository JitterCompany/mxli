#include <bufferWriter.h>
#include <string.h>

void bufferWriteN(BufferWriter *writer, const void *data, size_t n) {
	memcpy(writer->buffer+writer->position,data,n);
	writer->position += n;
}
