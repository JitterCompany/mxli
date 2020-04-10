/*
  parse.h - something similar to C's ctype.h

  Copyright 2014 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */
#ifndef __parse_h
#define __parse_h

/** @file
 * @brief Character classification functions, mostly inline functions.
 */

#include <stdbool.h>

/** Check for a digit.
 * @param c character
 * @return true, if c is in the range '0'..'9'.
 */
inline static bool isDigit(char c) {
	return '0'<=c && c<='9';
}

/** Checks for a binary digit.
 * @param c character
 * @return true, if c is in the range '0'..'9', 'a'..'f', 'A'..'F'
 */
inline static bool isBinDigit(char c) {
	return	c=='0' || c=='1';
}

/** Checks for a hexadecimal digit.
 * @param c character
 * @return true, if c is in the range '0'..'9', 'a'..'f', 'A'..'F'
 */
inline static bool isHexDigit(char c) {
	return	'0'<=c && c<='9'
		|| 'a'<=c && c<='f'
		|| 'A'<=c && c<='F';
}

/** Checks for a letter, that may be used within numbers to improve legibility.
 * At the moment, the only char is '_' (Thank you Larry Wall).
 */
inline static bool isLegible (char c) {
	return c == '_';
}

/** Check for a letter. Not really C standard complient.
 * @param c character
 * @return true, if c is in the range 'a'..'z' or 'A'..'Z'.
 */
inline static bool isAlpha(char c) {
	return 'a'<=c && c<='z' || 'A'<=c && c<='Z';
}

/** Check for a space character.
 * @param c character
 * @return true, if c is one of ' ', '\\t'
 */
inline static bool isBlank(char c) {
	return c==' ' || c=='\t';
}

/** Check for a space character.
 * @param c character
 * @return true, if c is one of ' ', '\\t', '\\f', '\\r', '\\n', ...
 */
bool isSpace(char c);

/** Converts a dec, hex or baseN digit to an integer.
 * @param baseNDigit a digit or uppercase/lowercase character.
 * @return the numeric value of the digit
 */
int baseNDigitToInt(char baseNDigit);

#endif
