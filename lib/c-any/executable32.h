/*
  executable32.h - 32-bit, multi-segment executable images (ARM typically). 
  Copyright 2014 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __executable32_h
#define __executable32_h

/** @file
 * @brief 32-bit, multi-segment executable images (ARM typically).
 *
 * A multi-segment image can be represented by an array of Segment32s. A segment of size 0 indicates the end of the
 * list.
 */
#include <integers.h>
#include <fifo.h>

#include <fifoParse.h>
#include <fifoPrint.h>
#include <simpleMath.h>

/** Contiguous memory block descriptor - data is stored in some other buffer.
 */
typedef struct {
	Uint32	size;		///< segment size; if 0, then it's the end of the segment list
	Uint32	address;
	Uint32	*data;
} Executable32Segment;

/** Returns the number of (non-empty!) segments of the list.
 */
Uint32 executable32Segments (const Executable32Segment *segs);

typedef struct {
	Uint32	segment;
	Uint32	offset;
} Executable32SegmentIterator;

/** Retrieves the next (contiguous) chunk of the executable.
 * @param exe the whole executable image
 * @param iterator the current position
 * @param chunkSize the maximum size of the chunk to return. The actual size may be smaller because of segment
 *   boundaries or image end.
 * @return a chunk (segment) of non-zero size if image data available or a zero-sized chunk at the end-of-image.
 */
Executable32Segment executable32NextChunk (
	const Executable32Segment *exe,
	Executable32SegmentIterator *iterator,
	Uint32 chunkSize);

/** Prints human-readable information about the executable.
 * @param fifo the output destination
 * @param segments a list of segments.
 * @return true if everything went right, false if output overflown.
 */
bool fifoPrintExecutable32Segments (Fifo *fifo, const Executable32Segment *segments);

#endif
