/*
  int32PairList.h 
  Copyright 2006-2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __int32PairList_h
#define __int32PairList_h

#include <stdbool.h>
#include <string.h>
#include <integers.h>
#include <fifoPrint.h>
#include <fifoParse.h>
#include <int32Pair.h>

/** @file
 * @brief Operations for lists of pairs of Int32/Uint32.
 *
 */

typedef struct {
	Int32Pair*	elements;
	Uint32		size;		///< size of elements-array in bytes; Needs 8-byte alignment
	Uint32		length;		///< used length in bytes
} Int32PairList;

typedef struct {
	Uint32Pair*	elements;
	Uint32		size;		///< size of elements-array in bytes; Needs 8-byte alignment
	Uint32		length;		///< used length in bytes
} Uint32PairList;

inline static void int32PairListClear (Int32PairList *list) {
	list->length = 0;
}

/** Creates a value copy of a list.
 */
bool int32PairListCopy (Int32PairList *dst, const Int32PairList *src);

inline static bool int32PairListAdd (Int32PairList *list, const Int32Pair *value) {
	if ( list->length + sizeof(Int32Pair) < list->size ) {
		list->elements [ list->length/sizeof(Int32Pair) ] = *value;
		list->length += sizeof (Int32Pair);
		return true;
	}
	else return false;
}

inline static const Int32PairList int32PairListTail (const Int32PairList *list) {
	Int32PairList l = {
		.size = list->size >= sizeof(Int32Pair) ? list->size-sizeof(Int32Pair) : 0,
		.elements = list->elements+1,
		.length = list->length >= sizeof(Int32Pair) ? list->length-sizeof(Int32Pair) : 0,
	};
	return l;
}

inline static int int32PairListLength (const Int32PairList *list) {
	return list->length / sizeof(Int32Pair);
}

inline static int int32PairListSize (const Int32PairList *list) {
	return list->size / sizeof(Int32Pair);
}

/** Returns the first pair in the list. Undefined for an empty list
 */
inline static const Int32Pair* int32PairListHead (const Int32PairList *list) {
	return list->elements;
}

inline static const Int32Pair* int32PairListAt (const Int32PairList *list, int index) {
	return & list->elements[index];
}

/** Returns the last pair in the list. Undefined for an empty list
 */
inline static const Int32Pair* int32PairListLast (const Int32PairList *list) {
	const int length = int32PairListLength (list);
	return & list->elements [length >= 1 ? length-1 : 0];
}

/** Replaces consecutive pairs (a,b), (b,c) by a single (a,c).
 * @return the negative number of fusions.
 */
int int32PairListTransitiveFusion (Int32PairList *list);

/** Replaces consecutive pairs (a,b), (b+1,c) by a single (a,c).
 * @return the negative number of fusions.
 */
int int32PairListSequenceFusion (Int32PairList *list);


bool fifoPrintInt32PairList (
	Fifo *fifo, const Int32PairList *list, bool (*printPair)(Fifo *, Int32Pair const*), char separator);

/** Parses a list of integers and appends to the result to the current list. A list may be empty, so parsing always
 * succeeds (at least for 0 elements). Take care to clear the list before using this function, if you need the full
 * capacity of the list for parsing. NOT THREAD safe.
 * @param fifo the text input
 * @param list the destination list. Results are appended.
 * @param parsePair a parser function for one single Int32Pair.
 * @param separator the list element separator char in the input. A (non-empty) list may end with that character. This
 *   is similar to the 'C'-syntax of initializer lists.
 * @return the number of list elements successfully parsed and added to the result list.
 */
int fifoParseInt32PairList (Fifo *fifo, Int32PairList *list, bool (*parsePair)(Fifo *fifo, Int32Pair*), char separator);

/** Parses a list of integers and appends to the result to the current list. A list may be empty, so parsing always
 * succeeds (at least for 0 elements). Take care to clear the list before using this function, if you need the full
 * capacity of the list for parsing. NOT THREAD safe.
 * @param fifo the text input
 * @param list the destination list. Results are appended.
 * @param parsePair a parser for one single Int32Pair.
 * @param separator the list element separator char in the input. A (non-empty) list may end with that character. This
 *   is similar to the 'C'-syntax of initializer lists.
 * @return the number of list elements successfully parsed and added to the result list.
 */
int fifoParseInt32PairListDynamic (Fifo *fifo, Int32PairList *list, const FifoParseInt32Pair *parsePair, char separator);

/** Parses a list of integer pairs and appends to the result to the current list. A list may be empty, so parsing always
 * succeeds (at least for 0 elements). Take care to clear the list before using this function, if you need the full
 * capacity of the list for parsing. NOT THREAD safe.
 * @param fifo the text input
 * @param list the destination list. Results are appended.
 * @param parsePair a parser for one single Int32Pair.
 * @param separator the list element separator char in the input. A (non-empty) list may end with that character. This
 *   is similar to the 'C'-syntax of initializer lists.
 * @return the number of list elements successfully parsed and added to the result list.
 */
int fifoParseInt32PairListWithInt32Parameter (
	Fifo *fifo, Int32PairList *list, const FifoParseInt32PairWithInt32Parameter *parsePair, char separator);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Uint32 type conversions
//

inline static void uint32PairListClear (Uint32PairList *list) {
	list->length = 0;
}

inline static bool uint32PairListAdd (Uint32PairList *list, const Uint32Pair *value) {
	return int32PairListAdd ((Int32PairList*)list,(const Int32Pair*) value);
}

inline static const Uint32PairList uint32PairListTail (const Uint32PairList *list) {
	Uint32PairList l = {
		.size = list->size >= sizeof(Uint32Pair) ? list->size-sizeof(Uint32Pair) : 0,
		.elements = list->elements+1,
		.length = list->length >= sizeof(Uint32Pair) ? list->length-sizeof(Uint32Pair) : 0,
	};
	return l;
}

inline static int uint32PairListLength (const Uint32PairList *list) {
	return int32PairListLength ((const Int32PairList*)list);
}

inline static int uint32PairListSize (const Uint32PairList *list) {
	return int32PairListSize ((const Int32PairList*)list);
}

inline static const Uint32Pair* uint32PairListAt (const Uint32PairList *list, int index) {
	return (Uint32Pair*) int32PairListAt ((const Int32PairList*)list,index);
}

/** Replaces consecutive pairs (a,b), (b,c) by a single (a,c).
 * @return the negative number of fusions.
 */
inline static int uint32PairListTransitiveFusion (Uint32PairList *list) {
	return int32PairListTransitiveFusion ((Int32PairList*)list);
}

inline static bool fifoPrintUint32PairList (
	Fifo *fifo, const Uint32PairList *list, bool (*printPair)(Fifo *, Uint32Pair const*), char separator) {

	return fifoPrintInt32PairList (
		fifo,
		(const Int32PairList*)list,
		(bool(*)(Fifo*,Int32Pair const*)) printPair,
		separator );
}

inline static int fifoParseUint32PairList (
	Fifo *fifo, Int32PairList *list, bool (*parsePair)(Fifo *fifo, Int32Pair*), char separator) {

	return fifoParseInt32PairList (
		fifo,
		(Int32PairList*)list,
		(bool(*)(Fifo*,Int32Pair *)) parsePair,
		separator );
}

#endif

