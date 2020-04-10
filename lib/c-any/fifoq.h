/*
  fifoq.h - a strange kind of string list.
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __fifoq_h
#define __fifoq_h

/** @file
 * @brief String list (queue) based on Fifo.
 *
 * A Fifo can be used to implement a (somehow lazy) sequence of 0-terminated strings. The end of the list can grow
 * and more elements become available later. The last element may grow until the list separator is found. 
 * Similarly adding an element at the end may fail at a moment but may succeed in the future.
 * To account for that the functions listed here are called printing and parsing functions rather than writing and
 * reading functions. Keep in mind, that parsing functions are destructive and should most likely be performed on
 * a clone, rather than on the original, if there are concurrent writes on the list. For accumulating new elements the
 * fifoPrint* functions are appropriate, while fifoParse* functions suit for parsing the head - as long as they do not
 * consume the list separator char.
 */

#include <fifo.h>
#include <fifoParse.h>
#include <fifoPrint.h>

enum {
	FIFO_Q_LIST_SEPARATOR=0,	///< Should be 0 for easy interoperability with C-strings.
};

typedef Fifo Fifoq;	///< introduced to render prototypes more readable

/** Checks if at least one element is (in completeness) available from the head of the list.
 */
bool fifoqHasHead(Fifoq const* q);

/** Returns the head element of the list without removing it.
 * Remove the head using @see fifoqSkipToNext.
 * @return the head of the list - if complete or not.
 */
Fifo fifoqHead(Fifoq const* q);

/** Parses and removes the head of the queue.
 * @param q the string list object.
 * @param head a Fifo variable, without an allocated buffer. In case of success it contains the (former) head of the
 * queue, whose buffer is no longer protected from overwriting by the queue.
 * @return true if head was complete and available, false otherwise.
 */
static inline bool fifoqParseHead(Fifoq* q, Fifo* head) {
	return fifoParseStringZ(q,head);
}

static inline bool fifoqPrintNext(Fifoq* q) {
	return fifoPrintChar(q,FIFO_Q_LIST_SEPARATOR);
}

/** Parses the list separator char (0).
 * @return true if 0 could be immediately read, false otherwise.
 */
static inline bool fifoqParseNext(Fifoq* q) {
	return fifoParseExactChar(q,FIFO_Q_LIST_SEPARATOR); 
}

/** Consumes everything up to (and including) the next list separator.
 * @return true if list separator was found and removed, false otherwise and Fifoq unchanged.
 */
bool fifoqParseSkipToNext(Fifoq* q);

/** Copies a string element into the queue as one element.
 * @param q the Fifoq queue
 * @param s a string element. 
 * @return true, if successful, false if capacity exhausted (and queue unchanged).
 */
bool fifoqPrintString(Fifoq* q, const char* s);

/** Copies the contents of a Fifo (a String element) into the queue as one element.
 * @param q the Fifoq queue
 * @param s a string element. MUST NOT contain any list separator.
 * @return true, if successful, false if capacity exhausted (and queue unchanged).
 */
bool fifoqPrintFifo(Fifoq* q, Fifo *s);

/** Counts the string elements of the queue.
 * @param q the Fifoq queue
 * @return the number of elements currently defined.
 */
size_t fifoqCount (const Fifoq *q);

#endif

