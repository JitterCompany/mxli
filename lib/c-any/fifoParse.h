/*
  fifoParse.h - text parsing functionality based on Fifo structure. 
  Copyright 2012-2014 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __fifoParse_h
#define __fifoParse_h

/** @file
 * @brief Fifo-based functions for parsing text.
 *
 * Recursive descent parsers. Every function operates on a Fifo and returns a bool that indicates, if the parse was
 * successful. The destination parameters are modified in case of success und unmodified in case of a failed parse.
 * Parsing is thread safe in the same meaning as Fifo itself: concurrent writing and parsing is safe.
 */

#include <parse.h>
#include <fifo.h>

/** Removes spaces and TABs.
 * @param fifo text input fifo.
 * @return true, if at least one space or TAB was found.
 */
bool fifoParseBlanks(Fifo *fifo);

/** Removes spaces, tabs, line-feeds, etc. by advancing the given fifo.
 * @param fifo text input fifo.
 * @return true, if there was at least one of these space chars, false otherwise
 */
bool fifoParseSpaces(Fifo *fifo);

/** Parses a single digit into a number.
 * @param fifo text input fifo.
 * @param value output destination. Not modified, if parse fails.
 * @return true if there was a digit, false if not.
 */
bool fifoParseDigit(Fifo *fifo, int *value);

/** Parses a single hex digit into a number.
 * @param fifo text input fifo.
 * @param value output destination. Not modified, if parse fails.
 * @return true if there was a digit, false if not.
 */
bool fifoParseHexDigit(Fifo *fifo, int *value);

/** Checks for a single line end. Line ends can be "\r" or "\n" or "\r\n". If the input stream ends after a single "\r"
 * then it is impossible to know, if a "\n" (Windows line marker) or a "\r" (successive Macintosh line marker) will
 * follow. In order to avoid blocking, the parsing will succeed at the first "\r" already.
 * @param fifo text input fifo.
 * @return true, if a line end was found, false otherwise
 */
bool fifoParseLineEnd(Fifo *fifo);

typedef enum {
	EOL_AUTO,	///< best effort auto detect. Flaw: detects a new line if "\r" at end of line even in DOS mode.
	EOL_LF,		///< only a "\n" will be recognized as a line end.
	EOL_CR,		///< only a "\r" will be recognized as a line end.
	EOL_CRLF,	///< only a "\r" followed by a "\n" will be recogized as a line end.
	EOL_CR_OR_LF,	///< both "\r" and "\n" will be recognized as a line end. CRLF translates into 2 line ends.
} FifoEolStyle;

/** Auto-dectect line parser state.
 */
typedef char FifoParseLineAuto;

/** Accumulates everything up to (not including) a line end. CR, LF or CRLF are accepted as line end sequences and each
 * of these causes one line to be recognized. This is performed by the simple rules: every CR causes a new line, every
 * LF not following a CR causes a new line.
 * @param fifo text input fifo.
 * @param state the last character that caused a new line. Initialize to 0.
 * @param line a Fifo to hold the result. This fifo should not own a buffer.
 * @return true, if line end was found, false otherwise (and Fifo is unchanged).
 */
bool fifoParseLineAuto(Fifo *fifo, FifoParseLineAuto *state, Fifo *line);

/** Accumulates everything up to (not including) a line end.
 * @param fifo text input fifo.
 * @param fifoLine a Fifo to hold the result. This fifo should not own a buffer.
 * @param eolStyle the end-of-line-style of the input (DOS/Unix/Mac/..).
 * @return true, if line end was found, false otherwise (and Fifo is unchanged).
 */
bool fifoParseLine(Fifo *fifo, Fifo *fifoLine, FifoEolStyle eolStyle);

/** Accumulates all characters found until a separator char is found.
 * @param fifo text input fifo.
 * @param fifoUntil a Fifo to hold the result. This fifo should not own a buffer. This may be the same as the input
 *   Fifo for an in-place update (chopping of everything from the separator char).
 * @param separators a set of separator chars. Must not be NULL.
 * @return true if one of the separator chars was found false otherwise (and input fifo is unchanged). 
 */
bool fifoParseUntil(Fifo *fifo, Fifo *fifoUntil, const char *separators);

/** Accumulates all characters found until the end.
 * @param fifo text input fifo.
 * @param fifoString a Fifo to hold the result. This fifo should not own a buffer. This may be the same as the input
 *   Fifo for an in-place update (chopping of everything from the separator char).
 * @return true if a non-empty string was found before the end of the fifo, false otherwise. 
 */
bool fifoParseStringNonEmpty(Fifo *fifo, Fifo *fifoString);

/** Parse a word consiting of characters matching a predicate.
 * @param fifo text input fifo.
 * @param fifoWord a Fifo containing the matched characters.
 * @param predicate a function that returns true for the allowed characters only.
 * @return true, if line end was found, false otherwise (and Fifo is unchanged).
 */
bool fifoParseWord(Fifo *fifo, Fifo *fifoWord, bool (*predicate)(char));

/** Extracts a given number of characters.
 * @param fifo text input fifo
 * @param fifoOutput an uninitialized Fifo structure to hold the result in case of success.
 * @param n the number of characters to extract
 * @return true, if enough characters are found, false otherwise (and input Fifo is unchanged).
 */
bool fifoParseN(Fifo *fifo, Fifo *fifoOutput, size_t n);

/** Parse a single char out of a given set.
 * @param fifo text input fifo.
 * @param set the set of allowed characters.
 * @param c an address to store the character found.
 * @return true in case of success, false otherwise (and fifo is restored).
 */
bool fifoParseCharSet(Fifo *fifo, char *c, const char *set);

/** Parse a single character only.
 * @param fifo text input fifo.
 * @param c the only allowed (and required!) character.
 * @return true in case of success, false otherwise (and fifo is restored).
 */
bool fifoParseExactChar(Fifo *fifo, char c);

/** Parses a single zero character. Fifos can contain any number of 0-characters like any other character. Hence it is
 * possible to store a list of C-strings in a fifo. This functions parses the C-string separator.
 * @param fifo text input fifo.
 * @return true, if the next character is available and is the 0-character, false otherwise.
 */
bool fifoParseZ(Fifo *fifo);

/** Parses a string up to a zero character. The zero character is removed from the input.
 * @param fifo text input fifo.
 * @param string a fifo for holding the result. Just a Fifo variable, without allocated buffer. In case of success this
 *   Fifo hold the result, not including the trailing 0-character.
 * @return true, if a terminating 0-character was found, false otherwise (and fifo unchanged).
 */
bool fifoParseStringZ(Fifo *fifo, Fifo *string);

/** Parse a given word - in other words: insist on word to show up in the input now and here, otherwise fail.
 * @param fifo text input fifo.
 * @param word the word that is parsed for.
 * @return true in case of success, false otherwise (and fifo is restored).
 */
bool fifoParseExactString(Fifo *fifo, const char *word);

/** Parses a boolean value and updatess fifo position in case of success. The 2 possible values are "true" and "false".
 * @param fifo text input fifo
 * @param value destination of parsed value
 * @return true in case of success, false otherwise (and fifo is restored).
 */
bool fifoParseBool(Fifo *fifo, bool *value);

/** Parses a boolean value and updatess fifo position in case of success. The 2 possible values are "true" and "false".
 * This function is handy for parsing things like "on" and "off" or "enable" and "disable", etc.
 * @param fifo text input fifo
 * @param value destination of parsed value
 * @param symbolFalse the string parsed as false (0).
 * @param symbolTrue the string parsed as true (1).
 * @return true in case of success, false otherwise (and fifo is restored).
 */
bool fifoParseBoolNamed(Fifo *fifo, const char *symbolFalse, const char *symbolTrue, bool *value);

/** Convenience function: keywords are 'off' and 'on'.
 */
bool fifoParseBoolOffOn(Fifo *fifo, bool *value);

/** Convenience function: keywords are 'no' and 'yes'.
 */
bool fifoParseBoolNoYes(Fifo *fifo, bool *value);

/** Parses a non-negative enum value. The enums are given in an array.
 * @param fifo text input Fifo
 * @param symbols an array of strings, each of which is the symbol for one enum. Make sure that no small index value
 *   is a prefix of a larger index value.
 * @param nSymbols number of symbols in the array.
 * @param value the output value, which is set in case of a successful match.
 * @return true in case of success, false otherwise (and fifo is restored).
 */
bool fifoParseEnum(Fifo *fifo, const char* const* symbols, int nSymbols, int *value);

/** Parses an hexadecimal string (without a prefix like 0x) and update fifo position in case of success. Otherwise the
 * buffer's position is unchanged.
 * @param fifo an fifo over the input string.
 * @param value destination
 * @return true, if an integer is successfully read, false otherwise.
 */
bool fifoParseHex(Fifo *fifo, unsigned *value);

/** Parses an hexadecimal string (without a prefix like 0x) and update fifo position in case of success. Otherwise the
 * buffer's position is unchanged.
 * @param fifo an fifo over the input string.
 * @param value destination
 * @param minDigits minimum number of hex digits required.
 * @param maxDigits maximum number of hex digits that will be read.
 * @return true, if an integer is successfully read, false otherwise.
 */
bool fifoParseHexN(Fifo *fifo, unsigned *value, int minDigits, int maxDigits);

/** Parses an hexadecimal string (without a prefix like 0x) and updates fifo position in case of success. Otherwise the
 * buffer's position is unchanged. I'm unsure, if this function is really needed.
 * @param fifo an fifo over the input string.
 * @param value destination
 * @param minimum the minimum value for a successful parse
 * @param maximum the maximum value for a successful parse
 * @return true, if an integer is successfully read, false otherwise.
 */
bool fifoParseHexLimited(Fifo *fifo, unsigned *value, unsigned minimum, unsigned maximum);

/** Parses a binary string (without a prefix like 0b) and updates fifo position in case of success. Otherwise the
 * buffer's position is unchanged.
 * @param fifo an fifo over the input string.
 * @param value destination
 * @return true, if an integer is successfully read, false otherwise.
 */
bool fifoParseBin(Fifo *fifo, unsigned *value);

/** Parses a binary string (without a prefix like 0b) and updates fifo position in case of success. Otherwise the
 * buffer's position is unchanged.
 * @param fifo an fifo over the input string.
 * @param value destination
 * @param minDigits minimum number of binary digits required.
 * @param maxDigits maximum number of binary digits that will be read.
 * @return true, if an integer is successfully read, false otherwise.
 */
bool fifoParseBinN(Fifo *fifo, unsigned *value, int minDigits, int maxDigits);

/** Parses an unsigned decimal from a string and update fifo position in case of success. Otherwise the buffer's
 * position is unchanged.
 * @param fifo an fifo over the input string.
 * @param value destination
 * @return true, if an integer is successfully read, false otherwise.
 * @deprecated Limits should be added
 */
bool fifoParseUnsigned(Fifo *fifo, unsigned *value);

/** Parses an unsigned decimal from a string and update fifo position in case of success. Otherwise the buffer's
 * position is unchanged.
 * @param fifo an fifo over the input string.
 * @param value destination
 * @param minimum the minimum value for a successful parse
 * @param maximum the maximum value for a successful parse
 * @return true, if an integer is successfully read, false otherwise.
 */
bool fifoParseUnsignedLimited(Fifo *fifo, unsigned *value, unsigned minimum, unsigned maximum);

/** Parse a decimal int from a string and update fifo position in case of success. Otherwise the buffer's position is
 * unchanged.
 * @param fifo an fifo over the input string.
 * @param value destination
 * @return true, if an integer is successfully read, false otherwise.
 * @deprecated Limits should be added
 */
bool fifoParseInt(Fifo *fifo, int *value);

/** Parse a decimal int from a string and update fifo position in case of success. Otherwise the buffer's position is
 * unchanged.
 * @param fifo a fifo over the input string.
 * @param value destination
 * @param minimum the minimum value for a successful parse
 * @param maximum the maximum value for a successful parse
 * @return true, if an integer is successfully read, false otherwise.
 */
bool fifoParseIntLimited(Fifo *fifo, int *value, int minimum, int maximum);

/** Parses an C-style int from a string and updates fifo position in case of success. Different number formats are
 * recognized: normal decimal integers or 0x hexadecimal values or 0b binary values.
 * @param fifo a fifo containing the input string.
 * @param value destination
 * @return true, if an integer is successfully read, false otherwise (and fifo's position unchanged).
 */
bool fifoParseIntCStyle (Fifo *fifo, int *value);

/** Engineer's style of numbers integer parsing. Allowed postfixes as multiplies are k,M,G, ki,Mi,Gi. The number
 * itself can be decimal or hex (0x prefix) or binary (0b prefix).
 * @param fifo a fifo containing the input string.
 * @param value destination
 * @return true, if an integer is successfully read, false otherwise (and fifo's position unchanged).
 */
bool fifoParseIntEng(Fifo *fifo, int *value);

/** Parse technical multipliers like M and a String unit, examples: "mA", "kV", "MHz". An unsuccessful parse restores
 * the initial state of the input. The allowed multipiers are y (yokto, 1E-24), z (zepto, 1E-21), a (atto, 1E-18),
 * f (femto, 1E-15), p (pico, 1E-12), n (nano, 1E-9), u (micro, 1E-6), m (milli, 1E-3), k (kilo, 1E+3), M (Mega, 1E+6),
 * G (Giga, 1E+9), T (Tera, 1E+12), P (Peta, 1E+15), E (Exa, 1E+18), Z (Zetta, 1E+21), Y (Yotta, 1E+24).
 *
 * @param fifo string input
 * @param power a pointer to a signed integer to hold the power, examples: m -> -3, k -> +3, M -> 6
 * @param unit a string containing the unit, examples: "A", "V", "Hz" or "" or 0 for empty unit.
 * @return true for a successful parse, false otherwise 
 */
bool fifoParseTechnicalUnit(Fifo *fifo, int *power, const char *unit);

/** Parse a technical value, like 8.25uF as a float value.
 * @param fifo string input
 * @param f a pointer to a float to hold the result
 * @param unit the unit to parse, like F or Hz .
 * @return true upon success, false otherwise (and fifo restored).
 */
bool fifoParseTechnicalFloat(Fifo *fifo, float *f, const char *unit);

/** Parses a value in units of u. 3u results in value=3, 5m results in value=5000 and 2 results in value=2000000.
 * @param fifo string input.
 * @param value a pointer to an unsigned to hold the result value.
 * @param unit a string unit, e.g. "g" for grams.
 * @return true upon success, false otherwise (and fifo is unchanged).
 */
bool fifoParseMicroUnit(Fifo *fifo, int *value, const char *unit);

/** Parses a (possibly) fractional decimal number.
 * @param fifo string input.
 * @param value a pointer to the integer to hold the result value.
 * @param scale the value of the result that is interpreted as 1.
 * @return true upon success, false otherwise (and fifo is unchanged).
 */
bool fifoParseFixedPointInt(Fifo *fifo, int *value, unsigned scale);

/** Returns the read contents of the Fifo as a C-string, if this contents is not wrapped around inside the Fifo.
 * @param fifo string input.
 * @return a pointer to the contents or 0 if wrap-around occurs.
 */
const char* fifoReadPositionToString(Fifo *fifo);


/** Checks if a given pattern can be read at the current position of a Fifo. It is not neccessary, that the pattern is
 * already available. It is sufficient, if there are no characters that prohibit the match.
 * @param fifo string input
 * @param pattern the exact pattern to match.
 * @return true if the chars available match the patter, false if at least one character does not match.
 */
bool fifoPartialMatch(Fifo *fifo, const char *pattern);

/** Searches for an exact pattern in the Fifo. The characters of the Fifo are consumed eventually. You cannot
 * search the input for different patterns one after another. This function is intended for removing leading characters
 * that CANNOT match in a scenario, where the Fifo slowly fills up with more characters.
 * @param fifo the input
 * @param pattern a string pattern to search for
 * @return true if pattern was found. All characters up to (including) the pattern are consumed.
 *   if pattern was not yet found, then any number of characters may be consumed.
 */
bool fifoSearch(Fifo *fifo, const char *pattern);

/** Searches for an exact pattern in the Fifo. All characters up to and including the pattern are consumed.
 * @param fifo the input
 * @param pattern a string pattern to search for. An empty pattern always matches, without consuming any characters.
 * @return true if pattern was found. All characters up to (including) the pattern are consumed.
 *   if pattern was not found, then no characters are consumed.
 */
bool fifoMatchUntilPattern(Fifo *fifo, const char *pattern);

/** Searches for an exact pattern in the Fifo. No charaters are consumed.
 * @param fifo the input
 * @param pattern a string pattern to search for. An empty pattern always matches.
 * @return true if pattern was found.
 */
bool fifoContainsPattern(Fifo *fifo, const char *pattern);

/** Searches for an exact pattern in the Fifo. No charaters are consumed.
 * @param fifo the input
 * @param pattern a character to search for.
 * @return true if pattern was found.
 */
bool fifoContainsChar(Fifo *fifo, char pattern);

/** Searches for an exact pattern in the Fifo. All characters up to and including the pattern are consumed.
 * @param fifo the input
 * @param pattern a character to search for.
 * @return true if pattern was found. All characters up to (including) the pattern are consumed.
 *   if pattern was not found, then no characters are consumed.
 */
bool fifoMatchUntilChar(Fifo *fifo, char pattern);

#endif
