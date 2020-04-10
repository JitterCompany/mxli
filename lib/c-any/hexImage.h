/*
  hexImage.h - loading Intel-Hex files.
  Copyright 2012 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __hexImage_h
#define __hexImage_h

/** @file
 * @brief Intel hex-file parsing.
 */
#include <integers.h>
#include <fifo.h>

#include <fifoParse.h>
#include <fifoPrint.h>
#include <simpleMath.h>
#include <executable32.h>

/** Contiguous memory block descriptor - data is stored in some other buffer.
 */
typedef struct {
	Uint32	size;
	Uint32	address;	///< address 0 is invalid
	Uint8	*data;
} Segment;

/** Checks, if a segment address is valid.
 */
static inline bool addressIsValid(Uint32 address) {
	return address!=0;
}

/** Data structure for building up hex images.
 */
typedef struct {
	Uint8*		ram;			///< elsewhere allocated memory
	Uint32		ramSize;		///< maximum size of memory
	Uint32		ramAddress;		///< linear address inside (contiguous) RAM
	Uint32		address;		///< linear address of next free byte
	Uint32		usedSegment;		///< number of used segments
	Uint32		addressSegment;		///< segment base address
	Uint32		entryPoint;		///< execution entry point
	Segment		segments[4];		///< list of segments
} HexImage;


/** Sets the segment address. This does not neccessarily create a new segment.
 */
inline static void hexImageSetSegmentAddress(HexImage *hi, Uint32 address) {
	hi->addressSegment = address;
}

/** Basically creates the initial segment.
 */
void hexImageInit(HexImage *hi);

static inline bool hexImageEmpty(const HexImage *hi) {
	return hi->usedSegment==0 && hi->segments[0].size==0;
}

/** Calculates the number of non-empty segments.
 * @param hi the hex image object.
 */
static inline bool hexImageSegments(const HexImage *hi) {
	return hexImageEmpty(hi) ? 0 : hi->usedSegment+1;
}

/** Writes a block of data.
 * @param hi the HexImage object
 * @param offset the offset withing the segment.
 * @param data the data bytes to write.
 * @param size the number of bytes in data.
 * @return true, if successfully written. False if either RAM or segment count is exceeded.
 */
bool hexImageWrite(HexImage *hi, Uint32 offset, const Uint8 *data, Uint32 size);

/** Prints out a segment's description without the data.
 * @param o test output.
 * @param s a hex-file segment.
 * @return true if completely written, false if output overflow.
 */
bool fifoPrintSegment(Fifo *o, const Segment *s);

/** Prints out a short description of a HexImage.
 * @param o test output.
 * @param hi the hex-file image.
 * @return true if completely written, false if output overflow.
 */
bool fifoPrintHexImage(Fifo *o, const HexImage *hi);

/** One line of a hex-file contains one HexRecord.
 */
typedef struct {
	Uint32	length;
	Uint32	offset;
	Uint32	type;
	Uint8	data[256];
	Uint32	checksum;
} HexRecord;

/** HexRecord types.
 */
typedef enum {
	HEX_DATA,
	HEX_EOF,
	HEX_EXTENDED_SEGMENT_ADDRESS,	///< defines the 16-B-segment address to be assumed for further data records
	HEX_START_SEGMENT,		///< CS:IP encoding of entry point
	HEX_EXTENDED_LINEAR_ADDRESS,	///< defined the 64Ki-segment address for further data records.
	HEX_START_LINEAR_ADDRESS,	///< 32-bit-IP encoding of entry point
} HexType;

bool fifoPrintHexType(Fifo *o, HexType type);

Uint32 hexRecordDataInt(const HexRecord *record, int width);

/** Extracts the linear address.
 */
Uint32 hexRecordAddressLinear(const HexRecord *record);

/** Extracts the 8086-segmented-style address as linear address.
 */
Uint32 hexRecordAddressSegmented(const HexRecord *record);

/** Extracts the linear address.
 */
Uint32 hexRecordStartLinear(const HexRecord *record);

/** Extracts the 8086-segmented-style address as linear address.
 */
Uint32 hexRecordStartSegmented(const HexRecord *record);

bool fifoPrintHexRecord(Fifo *o, const HexRecord *record);

bool fifoParseHexRecord(Fifo *fifo, HexRecord *record);

/** Appends the segments of hexImage to an executable.
 * @param segments an array of Segments. Should be empty, initially.
 * @param maxSegments the maximum number of segments, the executable can contain.
 * @param hexImage a hexImage with its more complex structure, compared to Executable32.
 * @param offset an offset added to all addresses of hexImage.
 * @return true, if conversion succeeded, false if segment number exceeded.
 */
bool executable32FromHexImage (Executable32Segment *segments, Uint32 maxSegments, const HexImage *hexImage, Uint32 offset);

#endif
