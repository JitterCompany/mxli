/*
  int32List.h 
  Copyright 2006-2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __int32List_h
#define __int32List_h

#include <stdbool.h>
#include <string.h>
#include <integers.h>
#include <fifoPrint.h>
#include <fifoParse.h>

/** @file
 * @brief Simple integer list operations.
 *
 */

typedef struct {
	Uint32*		elements;
	Uint32		size;		///< size of elements-array in bytes; Needs 4-byte alignment
	Uint32		length;		///< used length in bytes
} Uint32List;

typedef struct {
	Int32*		elements;
	Uint32		size;		///< size of elements-array in bytes; Needs 4-byte alignment
	Uint32		length;		///< used length in bytes
} Int32List;

inline static void int32ListClear (Int32List *list) {
	list->length = 0;
}

inline static bool int32ListAdd (Int32List *list, Int32 value) {
	if ( list->length + sizeof(Int32) < list->size ) {
		list->elements [ list->length/sizeof(Int32) ] = (Uint32)value;
		list->length += sizeof (Int32);
		return true;
	}
	else return false;
}

inline static Int32List int32ListTail (const Int32List *list) {
	Int32List l = {
		.size = list->size >= sizeof(Int32) ? list->size-sizeof(Int32) : 0,
		.elements = list->elements+1,
		.length = list->length >= sizeof(Int32) ? list->length-sizeof(Int32) : 0,
	};
	return l;
}

inline static int int32ListLength (const Int32List *list) {
	return list->length / sizeof(Int32);
}

inline static int int32ListSize (const Int32List *list) {
	return list->size / sizeof(Int32);
}

inline static Int32 int32ListHead (const Int32List *list) {
	return * list->elements;
}

inline static Int32 int32ListAt (const Int32List *list, int index) {
	return list->elements[index];
}

inline static Int32* int32ListReferenceAt (Int32List *list, int index) {
	return (Int32*) & list->elements[index];
}

inline static bool int32ListCopy(Int32List *dest, const Int32List *source) {
	int32ListClear(dest);
	bool success = true;
	for (int i=0; i<int32ListLength(source); i++) success = success && int32ListAdd(dest, int32ListAt(source,i));
	return success;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Uint32 types
//
inline static void uint32ListClear (Uint32List *list) {
	return int32ListClear ((Int32List*)list);
}

inline static bool uint32ListAdd (Int32List *list, Uint32 value) {
	return int32ListAdd (list,(Int32)value);
}

inline static Uint32List uint32ListTail (const Uint32List *list) {
	Uint32List l = {
		.size = list->size >= sizeof(Uint32) ? list->size-sizeof(Uint32) : 0,
		.elements = list->elements+1,
		.length = list->length >= sizeof(Uint32) ? list->length-sizeof(Uint32) : 0,
	};
	return l;
}

inline static int uint32ListLength (const Uint32List *list) {
	return int32ListLength ((const Int32List*)list);
}

inline static int uint32ListSize (const Uint32List *list) {
	return int32ListSize ((const Int32List*)list);
}

inline static Uint32 uint32ListAt (const Uint32List *list, int index) {
	return (Uint32) int32ListAt ((const Int32List*)list,index);
}

inline static Uint32* uint32ListReferenceAt (Uint32List *list, int index) {
	return (Uint32*) int32ListReferenceAt ((Int32List*)list,index);
}


bool fifoPrintInt32List (Fifo *fifo, const Int32List *list, bool (*printElement)(Fifo *, Int32), char separator);

/** Parses a list of integers and appends to the result to the current list. A list may be empty, so parsing always
 * succeeds (at least for 0 elements). Take care to clear the list before using this function, if you need the full
 * capacity of the list for parsing. NOT THREAD safe.
 * @param fifo the text input
 * @param list the destination list. Results are appended.
 * @param parseElement a parser function for one single integer element.
 * @param separator the list element separator char in the input. A (non-empty) list may end with that character. This
 *   is similar to the 'C'-syntax of initializer lists.
 * @return the number of list elements successfully parsed and added to the result list or -1 if list overflown.
 */
int fifoParseInt32List (Fifo *fifo, Int32List *list, bool (*parseElement)(Fifo *fifo, Int32*), char separator);

bool fifoPrintInt32ListDec (Fifo *fifo, Int32List *list);

bool fifoPrintUint32ListDec (Fifo *fifo, Uint32List *list);

bool fifoPrintUint32ListHex (Fifo *fifo, Uint32List *list);


#endif

