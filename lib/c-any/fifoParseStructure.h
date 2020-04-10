/*
  fifoParseStructure.h - Parsing a few composed types.
  Copyright 2012-2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __fifoParseStructure_h
#define __fifoParseStructure_h

/** @file
 * @brief Parsing of pairs, tuples, lists.
 */

#include <fifoParse.h>

typedef struct {
	int	a,b;
} PairInt;

typedef struct {
	bool		(*parseIntA)(Fifo *, int*);
	const char*	separator;
	bool		(*parseIntB)(Fifo*, int*);
	bool		(*validator)(Fifo *);
} FifoParsePairInt;

/** Parses a pair of integer values.
 */
bool fifoParsePairInt(Fifo *fifo, const FifoParsePairInt *parser, PairInt *p);

typedef struct {
	bool 		(*parseInt)(Fifo*, int*);
	const char*	separator;
	bool		(*validator)(Fifo*);
} FifoParseListInt;

/** Parses a list of integer values. BUG: The result reaches nMax-1 only.
 */
int fifoParseListInt(Fifo *fifo, const FifoParseListInt *parser, int *is, int nMax);

typedef struct {
	const FifoParsePairInt	*parsePair;
	const char*		separator;	///< list separator
	bool 			(*validator)(Fifo*);
} FifoParseListPairInt;

/** Parses a list of integer pairs. BUG: The result reaches nMax-1 only.
 */
int fifoParseListPairInt(Fifo *fifo, const FifoParseListPairInt* parser, PairInt *ps, int nMax);

/** Parses a masked hex value - a value with some hex-digits replaced by X (don't care) for example.
 * This is useful to specify some 8-bit portions of a 32-bit value.
 * @param fifo the text input
 * @param result the parsed result with the don't care bits set to 0
 * @param mask a bit pattern with 1s at valid position and 0s at don't care positions.
 * @param dontCare the character indicating a don't care nibble.
 * @return the number of digits parsed.
 */
int fifoParseHex32Masked(Fifo *fifo, Uint32 *result, Uint32 *mask, char dontCare);

////////////////////////////////////////////////////////////////////////////////////////////////////
// cleaner approach

enum FifoParseType {
	FIFO_PARSE_TYPE_EOL,		///< terminator of lists,
	FIFO_PARSE_TYPE_PRIMITIVE,	///< a primitive function
	FIFO_PARSE_TYPE_SEQUENCE,	///< all members are required, in sequence
	FIFO_PARSE_TYPE_ALTERNATIVE,	///< one member will be chosen
	FIFO_PARSE_TYPE_OPTIONAL,	///< any number of members, in sequence
};

typedef bool (*FifoParseFunction)(Fifo*);

struct FifoParse;
typedef struct FifoParse FifoParse;

struct FifoParse {
	int			type;
	union {
		const FifoParse* const*	ps;
		const FifoParseFunction	function;
	};
};

bool fifoParse(Fifo *fifo, const FifoParse *p);
bool fifoParseSequence(Fifo *fifo, const FifoParse* const* p);
bool fifoParseAlternative(Fifo *fifo, const FifoParse* const* p);
bool fifoParseOptional(Fifo *fifo, const FifoParse* const* p);

#endif
