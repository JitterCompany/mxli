/*
  fifoPopt.h - Program command line parsing using Fifo.
  Copyright 2012-2014 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef fifoPopt_h
#define fifoPopt_h

/** @file
 * @brief Command-line parsing using Fifo(q).
 *
 * c-any's Fifos can store '\0'-characters. This allows easy construction of lists of C-style strings. Fifoq is the
 * type that should be used for such string lists.
 * fifoPopt is based of lists of strings. A programs command line is translated into a list of strings. Every string
 * element can be an option (starting with a '-', for example "-g") or an argument (not starting with a '-', like for
 * example "/tmp/file.txt"). Things are in fact a bit more complicated than that, as some options may have an argument
 * (like "-o main.o"). Also, it is typically expected by a user to be able to write one single '-' and multiple
 * option characters as one word (like "-cgo main.o" instead of "-c -g -o main.o"). Most challenging is the fact, that
 * arguments may contain spaces and '-' characters if quoted strings are used as arguments.
 * This modules provides functionality to do all these things without too much effort by iterating over the Fifoq and
 * extracting parameters and options one by one.
 */
#include <integers.h>
#include <fifoq.h>
#include <fifoParseStructure.h>
#include <int32List.h>
#include <int32PairList.h>

/*
typedef struct {
	Int32	a;
	Int32	b;
} PairInt32;
*/

typedef bool FifoPoptValidator(Fifo *);

typedef struct {
	char		shortOption;	///< Option character (following the '-').
	const char*	longOption;	///< --longOption
	bool*		value;		///< where to write the parsed result?
} FifoPoptBool;


typedef struct {
	char		shortOption;	///< Option character (following the '-').
	const char*	longOption;	///< --optionString=
	Int32*		value;		///< where to write the indicator?
	Int32		indicator;	///< value is set to this if option is found.
} FifoPoptInt32Flag;


typedef bool (*FifoParseInt32)(Fifo*, Int32*);

typedef struct {
	char		shortOption;	///< Option character (following the '-').
	const char*	longOption;	///< --optionString=
	Int32*		value;		///< where to write the parsed result?
	FifoParseInt32	parseInt;	///< integer parser to apply, 0 = parseIntCStyle
} FifoPoptInt32;

typedef struct {
	char		shortOption;	///< Option charactor (following the '-').
	const char*	longOption;	///< --optionString=...
	Int32List*	value;		///< result list; will be truncated to 0 before parsing
	FifoParseInt32	parseInt;	///< integer parser to apply, 0 = parseIntCStyle
	char		separator;	///< list element separator char
} FifoPoptInt32List;

typedef struct {
	char		shortOption;	///< Option charactor (following the '-').
	const char*	longOption;	///< --optionString=...
	Int32PairList*	value;		///< result list; will be truncated to 0 before parsing
	FifoParseInt32	parseIntFst;	///< integer parser for first pair component, 0 = parseIntCStyle
	char		separatorPair;	///< Pair constructor char
	FifoParseInt32	parseIntSnd;	///< integer parser for second pair component, 0 = parseIntCStyle
	char		separatorList;	///< list constructor char, typically ','
} FifoPoptInt32PairList;

typedef bool (*FifoParseInt32PairListWithInt32ParameterFunction)(Fifo*, Int32Pair* resultList, Int32 parameter);

typedef struct {
	char							shortOption;	///< Option charactor (following the '-').
	const char*						longOption;	///< --optionString=...
	Int32PairList*						value;		///< result list; will be truncated to 0 before parsing
	FifoParseInt32PairListWithInt32ParameterFunction	parsePair;	///< pair parser 
	Int32							parameter;	///< don't care character, for example
	char							separator;	///< Pair constructor char
} FifoPoptInt32PairListWithInt32Parameter;

typedef struct {
	char		shortOption;	///< Option character (following the '-').
	const char*	longOption;	///< --optionString=
	Fifo*		value;		///< where to write the parsed result?
} FifoPoptString;

typedef struct {
	char		shortOption;	///< Option character (following the '-').
	const char*	longOption;	///< --optionString=
	Int32Pair*	value;		///< where to write the parsed result?
	const char*	separator;	///< range operator symbol, 0 = ..
	FifoParseInt32	parseInt;	///< integer parser to apply, 0 = parseIntCStyle
} FifoPoptInt32Range;

typedef struct {
	char			shortOption;	///< Option character (following the '-').
	const char*		longOption;	///< --optionString=
	int*			value;		///< where to write the parsed result?
	const char* const*	symbols;	///< array of literal symbols
} FifoPoptSymbol;

typedef struct {
	char		shortOption;	///< Option character
	const char*	longOption;	///< --optionString=
	int		n;		///< number of elements
	Int32*		values;		///< array of results
	const char*	separator;	///< separator, typically ","
	FifoParseInt32	parseInt;	///< integer parser to apply, 0 = parseIntCStyle
} FifoPoptInt32Tuple;

/** Scans for an option (short or long) without modifying the command line.
 * This is intended for the help option.
 * @param cmdLine the accumulated command line list.
 * @param shortOption the option character of the short option, e.g. '?' for -? .
 * @param longOption the option string of the long option, e.b. 'help' for --help. Set to 0 for 'no long Option'.
 * @return true, if the option is found, false otherwise.
 */
bool fifoPoptScanOption(Fifoq *cmdLine, char shortOption, const char* longOption);

/** Accumulates up a processes' command line into a single Fifo.
 * @param argList a Fifo for accumulating the parameter list.
 * @param argc the main()'s argc parameter
 * @param argv the main()'s argv parameter
 * @return true if all data fit into argList.
 */
bool fifoPoptAccumulateCommandLine(Fifoq *argList, int argc, const char* const *argv);

/** Parses an option char and its preceding '-' sign at the beginning of a single characters option block.
 * @param cmdLine a command line with the options separated by 0-characters.
 * @param option the option char
 * @param minus as input: indicates, that a - was already found in this block and we're searching for further
 *   option characters only.
 * @return true, if the option was found.
 */
bool fifoPoptShort(Fifoq *cmdLine, char option, bool minus);

/** Parses an option string and its prefix "--" signs.
 * @param cmdLine a command line with the options separated by 0-characters.
 * @param longOption the option string excluding the leading "--"
 * @param minus indicates, that a "-" was already found in this block and we're searching for further
 *   option characters only. This causes immediate failure of match.
 * @return true, if the option was found.
 */
bool fifoPoptLong(Fifoq *cmdLine, const char* longOption, bool minus);

/** Parses short or long option.
 */
bool fifoPoptShortOrLong(Fifoq *cmdLine, char shortOption, const char* longOption, bool minus);

/** Parses short or long option that is followed by a parameter.
 */
bool fifoPoptShortOrLongParam(Fifoq *fifo, char shortOption, const char* longOption, bool minus);

/** Parses one option character. At most, one string element is consumed. It is possible that some characters are
 * left over, which might be other option characters of a block or a character requiring a parameter. In that case
 * minus is set to true.
 * @param cmdLine a command line with the options separated by 0-characters.
 * @param options a zero-terminated array of option descriptors.
 * @param minus initially this value indicates, if a '-' was found before calling this function what means, were
 *   parsing a block of options. Upon leaving this function, the value is set if we're still in a block.
 * @return true if at least one option found and parsed correctly.
 */
bool fifoPoptBool(Fifoq *cmdLine, const FifoPoptBool *options, bool *minus);

/** Parses (possibly) accumulated options immediately (and in direct sequence) available. 
 * @param cmdLine a command line with the options separated by 0-characters.
 * @param options a zero-terminated array of option descriptors.
 * @param minus initially this value indicates, if a '-' was found before calling this function.
 *   Upon leaving this function, the value is set if a '-' is found in the input or was found before, false otherwise.
 * @return true if option found.
 */
bool fifoPoptInt32Flag(Fifoq *cmdLine, const FifoPoptInt32Flag *options, bool *minus);

/** Parses an int option immediately available.
 * @param cmdLine a command line with the options separated by 0-characters.
 * @param options a zero-terminated array of option descriptors.
 * @param minus initially this value indicates, if a '-' was found before calling this function.
 *   Upon leaving this function, the value is set if a '-' is found in the input or was found before, false otherwise.
 * @return true if option found and parsed correctly.
 */
bool fifoPoptInt32(Fifoq *cmdLine, const FifoPoptInt32 *options, bool *minus);

/** Parses an int option immediately available.
 * @param cmdLine a command line with the options separated by 0-characters.
 * @param options a zero-terminated array of option descriptors.
 * @param minus initially this value indicates, if a '-' was found before calling this function.
 *   Upon leaving this function, the value is set if a '-' is found in the input or was found before, false otherwise.
 * @return true if option found and parsed correctly.
 */
bool fifoPoptInt32List(Fifoq *cmdLine, const FifoPoptInt32List *options, bool *minus);

/** Parses an int (with dont cares) list immediately available.
 * @param cmdLine a command line with the options separated by 0-characters.
 * @param options a zero-terminated array of option descriptors.
 * @param minus initially this value indicates, if a '-' was found before calling this function.
 *   Upon leaving this function, the value is set if a '-' is found in the input or was found before, false otherwise.
 * @return true if option found and parsed correctly.
 */
bool fifoPoptInt32PairListWithInt32Parameter (
	Fifoq *cmdLine, const FifoPoptInt32PairListWithInt32Parameter *options, bool *minus);

/** Parses an int option immediately available.
 * @param cmdLine a command line with the options separated by 0-characters.
 * @param options a zero-terminated array of option descriptors.
 * @param minus initially this value indicates, if a '-' was found before calling this function.
 *   Upon leaving this function, the value is set if a '-' is found in the input or was found before, false otherwise.
 * @return true if option found and parsed correctly.
 */
bool fifoPoptInt32PairList(Fifoq *cmdLine, const FifoPoptInt32PairList *options, bool *minus);

/** Parses all string options immediately (and in direct sequence) available.
 * @param cmdLine a command line with the options separated by 0-characters.
 * @param options a zero-terminated array of option descriptors.
 * @param minus initially this value indicates, if a '-' was found before calling this function.
 *   Upon leaving this function, the value is set if a '-' is found in the input or was found before, false otherwise.
 * @return true if option found and parsed correctly.
 */
bool fifoPoptString(Fifoq *cmdLine, const FifoPoptString *options, bool *minus);

/** Extracts a non-option from the command line and puts it into the list of non-options. A 0-character is appended
 * after the non-option string to separate it from following strings. It should be noted, that the output fifo can be
 * derived from the input Fifo in such a way, that they share buffers and this function does an in-place update. For
 * that, the output Fifo must have the wPos set to the input's Fifo buffer start and wTotal=size and rTotal=0.
 * @param cmdLine a command line.
 * @param listOfNonOptions an (initialized) fifo for holding the non-option string list.
 * @param minus initially this value indicates, if a '-' was found before calling this function.
 *   Upon leaving this function, the value is set if a '-' is found in the input or was found before, false otherwise.
 * @return true, unless the output Fifo overflows.
 */
bool fifoPoptNonOptionAccumulate(Fifoq *cmdLine, Fifoq *listOfNonOptions, bool *minus);

/** Parses an int range immediately available.
 * @param cmdLine a command line with the options separated by 0-characters.
 * @param options a zero-terminated array of option descriptors.
 * @param minus initially this value indicates, if a '-' was found before calling this function.
 *   Upon leaving this function, the value is set if a '-' is found in the input or was found before, false otherwise.
 * @return true if option found and parsed correctly.
 */
bool fifoPoptInt32Range(Fifoq *cmdLine, const FifoPoptInt32Range *options, bool *minus);

/** Parses an option of a fixed size tuple of ints.
 * @param cmdLine a command line with the options separated by 0-characters.
 * @param options a zero-terminated array of option descriptors.
 * @param minus initially this value indicates, if a '-' was found before calling this function.
 *   Upon leaving this function, the value is set if a '-' is found in the input or was found before, false otherwise.
 * @return true if option found and parsed correctly.
 */
bool fifoPoptInt32Tuple(Fifoq *cmdLine, const FifoPoptInt32Tuple *options, bool *minus);

/** Parses a symbolic option immediately available.
 * @param cmdLine a command line with the options separated by 0-characters.
 * @param options a zero-terminated array of option descriptors.
 * @param minus initially this value indicates, if a '-' was found before calling this function.
 *   Upon leaving this function, the value is set if a '-' is found in the input or was found before, false otherwise.
 * @return true if option found and parsed correctly.
 */
bool fifoPoptSymbol(Fifoq *cmdLine, const FifoPoptSymbol* options, bool *minus);

#endif
