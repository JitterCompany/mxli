/*
  stringTable.h 
  Copyright 2016 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __stringTable_h
#define __stringTable_h

/** @file
 * @brief A simple, fixed-size collection for storing a limited number of C-style strings.
 *
 * Strings are copied into a char-array, 0-terminated and and returned as a pointer to the start of string's char
 * sequence. This allows a fixed-size structure element to hold a string of (nearly) arbitrary size. As a special
 * feature Fifo read contents can be used as input type, too.
 */

#include <stdbool.h>
#include <symbolTable.h>	// streq function
#include <fifo.h>

/** A simple, fixed-size collection for storing a limited number of C-style strings.
 */
struct StringTable {
	char*	buffer;		///< string, 0 at end of list
	size_t	bufferSize;
	size_t	wPos;		///< writing position = end of used space
};
typedef struct StringTable StringTable;

/** Efficient function for storing a string in the table. No duplicate elimination is performed.
 * @param stringTable the StringTable object to use
 * @param string a 0-terminated String to be copied into the table.
 * @return a pointer to the new copy of the string
 */
const char* stringTableWriteString (StringTable *stringTable, const char* string);
const char* stringTableWriteStringUnique (StringTable *stringTable, const char* string);

/** Efficient function for storing a string in the table. No duplicate elimination is performed.
 * @param stringTable the StringTable object to use
 * @param string a Fifo containing (only) the string to be copied into the table.
 * @return a pointer to the new copy of the string
 */
const char* stringTableWriteFifo (StringTable *stringTable, const Fifo *string);
const char* stringTableWriteFifoUnique (StringTable *stringTable, const Fifo *string);

/** Returns the first string in the table, if any
 * @param stringTable the StringTable object to use
 * @return a 0-terminated String (may be empty) or 0 if not a single string is stored in the table
 */
inline static const char* stringTableFirstString (const StringTable *stringTable) {
	return stringTable->wPos!=0 ? stringTable->buffer : 0;
}

/** Iterator advancement.
 * @param stringTable the StringTable object to use
 * @param string the current position. This MUST be a valid string pointer into the table.
 * @return a 0-terminated String (may be empty) or 0 if the end of the table is reached.
 */
const char* stringTableNextString (const StringTable *stringTable, const char* string);

/** Inefficient function for looking up a string in the table.
 * @param stringTable the StringTable object to use
 * @param string a 0-terminated String to be searched for.
 * @return a pointer to the copy of the string inside the table or 0 if no such string is found.
 */
const char* stringTableLookupString (StringTable *stringTable, const char *string);

/** Inefficient function for looking up a string in the table.
 * @param stringTable the StringTable object to use
 * @param string a Fifo containing (only) the string to be searched for.
 * @return a pointer to the copy of the string inside the table or 0 if no such string is found.
 */
const char* stringTableLookupFifo (StringTable *stringTable, const Fifo *string);

#endif
