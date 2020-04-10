//HEADER
/*
  hexImage-sliced.c 
  Copyright 2012 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <hexImage.h>

#include <string.h>
#include <macros.h>

#include <fifoParse.h>
#include <fifoPrint.h>
#include <simpleMath.h>

//SLICE
void hexImageInit(HexImage *hi) {
	hi->segments[0].size = 0;
	hi->segments[0].address = 0;
	hi->segments[0].data = hi->ram;
	hi->ramAddress = 0;
	hi->address = 0;
}

//SLICE
bool hexImageWrite(HexImage *hi, Uint32 offset, const Uint8 *data, Uint32 size) {
	const Uint32 address = hi->addressSegment + offset;
	if (address>hi->address) {	// non-contiguous write
		if (hi->segments[hi->usedSegment].size>0) {	// && previous segment is NOT empty
			if (hi->usedSegment+1<ELEMENTS(hi->segments)) {
				hi->usedSegment++;
				hi->segments[hi->usedSegment].size = 0;
				hi->segments[hi->usedSegment].data = hi->ram + hi->ramAddress;
				hi->address = address;
			}
			else return false;	// no more segments
		}
		else {		// previous segment is emtpy - recycle it
			hi->address = address;
		}
	}

	if (size>0 && hi->ramSize - hi->ramAddress >= size) {
		Segment *segment = &hi->segments[hi->usedSegment];
		if (segment->size==0) segment->address = address;	// lazy assignment of base address
		memcpy(hi->ram + hi->ramAddress, data, size);
		segment->size += size;
		hi->ramAddress += size;
		hi->address += size;
		return true;
	}
	else return false;
}

//SLICE
bool fifoPrintSegment(Fifo *o, const Segment *s) {
	return	fifoPrintString(o,"addr=0x") && fifoPrintHex(o,s->address,8,8)
		&& fifoPrintString(o,", size=0x") && fifoPrintHex(o,s->size,2,8)
		&& fifoPrintString(o,", data=0x") && fifoPrintHex(o,(Uint32)(Uint)s->data,2,8);
}

//SLICE
bool fifoPrintHexImage(Fifo *o, const HexImage *hi) {
	for (int s=0; s<=hi->usedSegment; ++s) {
		if (fifoPrintString(o,"segment ") && fifoPrintUDec(o,s,1,10)
		&& fifoPrintString(o,": ") && fifoPrintSegment(o,&hi->segments[s]) && fifoPrintLn(o) ); // fine
		else return false;
	}
	return fifoPrintString(o,"entryPoint=0x") && fifoPrintHex(o,hi->entryPoint,8,8);
}

//SLICE
bool fifoPrintHexType(Fifo *o, HexType type) {
	switch(type) {
		case HEX_DATA:				return fifoPrintString(o,"HEX_DATA");	
		case HEX_EOF:				return fifoPrintString(o,"HEX_EOF");
		case HEX_EXTENDED_SEGMENT_ADDRESS:	return fifoPrintString(o,"HEX_EXTENDED_SEGMENT_ADDRESS");
		case HEX_START_SEGMENT:			return fifoPrintString(o,"HEX_START_SEGMENT");
		case HEX_EXTENDED_LINEAR_ADDRESS:	return fifoPrintString(o,"HEX_EXTENDED_LINEAR_ADDRESS");
		case HEX_START_LINEAR_ADDRESS:		return fifoPrintString(o,"HEX_START_LINEAR_ADDRESS");
		default:				return fifoPrintString(o,"HEX UNDEFINED");
	}
}

//SLICE
Uint32 hexRecordDataInt(const HexRecord *record, int width) {
	int value = 0;
	for (int i=0; i<width; ++i) value |= record->data[i] << 8*i;

	return value;
}

//SLICE
Uint32 hexRecordAddressLinear(const HexRecord *record) {
	return (Uint32)changeEndian16(hexRecordDataInt(record,2)) << 16;
}

//SLICE
Uint32 hexRecordAddressSegmented(const HexRecord *record) {
	return (Uint32)changeEndian16(hexRecordDataInt(record,2)) << 4;	// old 8086 16-byte segments
}

//SLICE
Uint32 hexRecordStartLinear(const HexRecord *record) {
	return (Uint32)changeEndian32(hexRecordDataInt(record,4));
}

//SLICE
Uint32 hexRecordStartSegmented(const HexRecord *record) {
	Uint32 csip = (Uint32)changeEndian32(hexRecordDataInt(record,4));
	return (csip>>12 & 0x000FFFF0) + (csip & 0xFFFF);
}

//SLICE
bool fifoPrintHexRecord(Fifo *o, const HexRecord *record) {
	bool success = fifoPrintString(o,"length=")
		&& fifoPrintUDec(o,record->length,1,10)
		&& fifoPrintString(o,", offset=0x")
		&& fifoPrintHex(o,record->offset,4,4)
		&& fifoPrintString(o,", type=")
		&& fifoPrintHexType(o, record->type);

	switch(record->type) {
		case HEX_DATA:	success = success
				&& fifoPrintString(o,", data=")
				&& fifoPrintHexString(o,(const char*)record->data,record->length,1);
				break;
		case HEX_EXTENDED_LINEAR_ADDRESS: success = success
				&& fifoPrintString(o,", ULBA=0x")
				&& fifoPrintHex(o,hexRecordAddressLinear(record),8,8);
				break;
		case HEX_EXTENDED_SEGMENT_ADDRESS: success = success
				&& fifoPrintString(o,", USBA=0x")
				&& fifoPrintHex(o,hexRecordAddressSegmented(record),8,8);
				break;
		case HEX_START_LINEAR_ADDRESS: success = success
				&& fifoPrintString(o,", START=0x")
				&& fifoPrintHex(o,hexRecordStartLinear(record),8,8);
				break;
		case HEX_START_SEGMENT: success = success
				&& fifoPrintString(o,", START=0x")
				&& fifoPrintHex(o,hexRecordStartSegmented(record),8,8);
				break;
		default:;
	}
	success = success
		&& fifoPrintString(o,", checksum=0x")
		&& fifoPrintHex(o,record->checksum,2,2);
	return success;
}

//SLICE
bool fifoParseHexRecord(Fifo *fifo, HexRecord *record) {
	Fifo clone = *fifo;
	if (fifoParseExactChar(&clone,':') && fifoCanRead(&clone)>5) {
		Uint32 length;
		if (fifoParseHexN(&clone,&length,2,2) && length<=255) {
			int checksum = 0;
			record->length = length; checksum += length;
			if (!fifoParseHexN(&clone,&record->offset,4,4)) return false;
			checksum += record->offset + (record->offset>>8);
			if (!fifoParseHexN(&clone,&record->type,2,2)) return false;
			checksum += record->type;
			for (int i = 0; i<length; i++) {
				unsigned value;
				if (!fifoParseHexN(&clone, &value, 2,2)) return false;
				record->data[i] = value;
				checksum += value;
			}
			if (!fifoParseHexN(&clone,&record->checksum,2,2)) return false;
			fifoCopyReadPosition(fifo,&clone);
			return true;	// checksum == record->checksum;
		}
		else return false;
	}
	else return false;
}

//SLICE
bool executable32FromHexImage (Executable32Segment *segments, Uint32 maxSegments, const HexImage *hexImage, Uint32 offset) {
	int s = 0;
	while (s<maxSegments && segments [s].size!=0) s++;	// find first free segment

	for (int his=0; his<hexImageSegments (hexImage); his++) {
		if (s+1<maxSegments) {
			segments [s].size	= hexImage->segments [his].size;
			segments [s].address	= hexImage->segments [his].address + offset;
			segments [s].data	= (Uint32*)hexImage->segments [his].data;
			segments [s+1].size	= 0;
		}
		else return false;	// destination buffer too small.
	}
	return true;
}
