/*
  int32Pair.h 
  Copyright 2006-2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __int32Pair_h
#define __int32Pair_h

#include <stdbool.h>
#include <string.h>
#include <integers.h>
#include <fifoPrint.h>
#include <fifoParse.h>
#include <unionized.h>

/** @file
 * @brief Simple Integer pair types.
 *
 */

typedef struct PACKED4 {
	Uint32		fst;
	Uint32		snd;
} Uint32Pair;

typedef struct PACKED4 {
	Int32		fst;
	Int32		snd;
} Int32Pair;

/** Parses a pair of integers.
 * @param fifo the text input
 * @param pair the result variable
 * @param parseElement a parser function for one single integer element.
 * @param separator the pair element separator char in the input.
 * @return true, if parsed successfully, false otherwise (and pair not changed).
 */
bool fifoParseInt32Pair (Fifo *fifo, Int32Pair *pair, bool (*parseElement)(Fifo *fifo, Int32*), char separator);


typedef struct {
	bool 	(*parseFst)(Fifo*, Int32*);
	char	separator;
	bool 	(*parseSnd)(Fifo*, Int32*);
} FifoParseInt32Pair;

/** Parses a pair of integers. The name comes from the possibility to create the parser dynamically at runtime.
 * @param fifo the text input
 * @param pair the result variable
 * @param parser a parser for the whole pair.
 * @return true, if parsed successfully, false otherwise (and pair not changed).
 */
bool fifoParseInt32PairDynamic (Fifo *fifo, Int32Pair *pair, FifoParseInt32Pair const *parser);



/** Prototype function: parses a pair of integers using a runtime parameter (don't care char or similar..).
 * @param fifo the text input
 * @param pair the result variable
 * @param parseElement a parser function for one single integer element.
 * @return true, if parsed successfully, false otherwise (and pair not changed).
 */
typedef bool FifoParseInt32PairWithInt32ParameterFunction (Fifo* fifo, Int32Pair* pair, Int32 parameter);

typedef struct {
	FifoParseInt32PairWithInt32ParameterFunction	*parse;
	Uint32						parameter;
} FifoParseInt32PairWithInt32Parameter;

/** Parses a pair of integers using a runtime parameter (don't care char or similar..).
 * @param fifo the text input
 * @param pair the result variable
 * @param parseElement a parser function for one single integer element.
 * @return true, if parsed successfully, false otherwise (and pair not changed).
 */
bool fifoParseInt32PairWithInt32Parameter (
	Fifo *fifo, Int32Pair *pair, FifoParseInt32PairWithInt32Parameter const *parseElement);


/** Type conversion inline function.
 */
inline static bool fifoParseUint32Pair (
	Fifo *fifo, Uint32Pair *pair, bool (*parseElement)(Fifo *fifo, Uint32*), char separator) {

	return fifoParseInt32Pair (fifo, (Int32Pair*)pair, (bool (*)(Fifo*,Int32*))parseElement, separator);
}

/** Parses a 32-bit integer with the freedom to use don't cares for some bits. The result is stored as two values in a
 * pair. The first value is the actual value while the second is a bit mask with a 1 for every important bit or a 0 for
 * a don't care bit. This function accepts either decimal input (and no don't cares) or hexadecimal input with nibble 
 * don't cares or binary input with single bit don't cares. Every don't care is denoted by an X. Providing less than
 * 32 bits of data results in the higher bits beeing don't cares, too.
 * @param fifo the text input
 * @param pair the result variable, field fst as value and snd as bit mask.
 * @param dontCare the don't care character.
 * @return true, if parsed successfully, false otherwise (and pair not changed).
 */
bool fifoParseInt32WithDontCares (Fifo *fifo, Int32Pair *pair, Int32 dontCare);

/** Parses a 32-bit integer with the freedom to use don't cares for some bits. The result is stored as two values in a
 * pair. The first value is the actual value while the second is a bit mask with a 1 for every important bit or a 0 for
 * a don't care bit. This function accepts binary input with single bit don't cares. Providing less than 32 digits
 * results in the higher bits beeing don't cares, too.
 * @param fifo the text input
 * @param pair the result variable, field fst as value and snd as bit mask.
 * @param dontCare the don't care character.
 * @return true, if parsed successfully, false otherwise (and pair not changed).
 */
bool fifoParseInt32BinWithDontCares (Fifo *fifo, Int32Pair *pair, Int32 dontCare);

/** Parses a 32-bit integer with the freedom to use don't cares for some bits. The result is stored as two values in a
 * pair. The first value is the actual value while the second is a bit mask with a 1 for every important bit or a 0 for
 * a don't care bit. This function accepts hexadecimal input with nibble don't cares. Providing less than 8 digits
 * results in the higher nibbles beeing don't cares, too.
 * @param fifo the text input
 * @param pair the result variable, field fst as value and snd as bit mask.
 * @param dontCare the don't care character.
 * @return true, if parsed successfully, false otherwise (and pair not changed).
 */
bool fifoParseInt32HexWithDontCares (Fifo *fifo, Int32Pair *pair, Int32 dontCare);

/** Checks, for correct don't cares alignment for hexadecimal output.
 * @param pair the value/don't care patterns
 * @return true if this number can be represented in hexadecimal, false if not (but in binary then...).
 */
bool canPrintInt32HexWithDontCares (const Int32Pair *pair);

/** Prints a 32-bit integer as a hex number, with all don't care nibbles replaced by the given don't care character.
 * @param fifo the text output
 * @param pair a pair with the value in component fst and the validation bits in snd (1=valid, 0=don't care).
 * @param dontCare the character used to indicate a don't care nibble
 * @return true if conversion succeeded, false if the number's don't cares cannot be represented in hex because of
 *   misalignment or if the output is exhausted. 
 */
bool fifoPrintInt32HexWithDontCares (Fifo *fifo, const Int32Pair *pair, char dontCare);

/** Prints a 32-bit integer as a binary number, with all don't care bits replaced by the given don't care character.
 * @param fifo the text output
 * @param pair a pair with the value in component fst and the validation bits in snd (1=valid, 0=don't care).
 * @param dontCare the character used to indicate a don't care bits
 * @return true if conversion succeeded, false if output exhausted.
 */
bool fifoPrintInt32BinWithDontCares (Fifo *fifo, const Int32Pair *pair, char dontCare);

#endif

