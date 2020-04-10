/*
  fifoPrint.h - Formatted output onto Fifo. 
  Copyright 2012-2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef fifoPrint_H
#define fifoPrint_H

/** @file
 * @brief Fifo-based formatting of text output.
 */

#include <stdbool.h>
#include <fifo.h>

extern const char siPrefixMultiplier3[9];	///< [0]=' '=1, [1]='k'=10^3, [2]='M'=10^6,...
extern const char siPrefixDivider3[9];		///< [0]=' '=1, [1]='m'=10^-3, [2]='u'=10^-6,...

/** Appends a single character at the end of the current buffer contents. Errors are indicated by ? (out of range)
 * and ! (not supported, like deca or deci).
 * @param fifo output destination.
 * @param exponent the SI decimal exponent, like 3 for 'k' (kilo, 1000) or -6 for 'u' (micro, 1E-6).
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
bool fifoPrintSiPrefix (Fifo *fifo, int exponent);

/** Append a single character at the end of the current buffer contents.
 * @param fifo output destination.
 * @param c the character to append.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
bool fifoPrintChar(Fifo *fifo, char c);

/** Append a single space character at the end of the current buffer contents.
 * @param fifo output destination.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
inline static bool fifoPrintSpace (Fifo *fifo) {
	return fifoPrintChar (fifo,' ');
}

/** Append a single line-feed (0xA) character at the end of the current buffer contents.
 * @param fifo output destination.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
inline static bool fifoPrintLf (Fifo *fifo) {
	return fifoPrintChar (fifo,'\n');
}

/** Append a character at the end of the current buffer contents. Control characters are printed as
 * escape sequences.
 * @param fifo output destination.
 * @param c the character to append.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
bool fifoPrintCharEscaped(Fifo *fifo, char c);

/** Append a string at the end of the current buffer contents.
 * @param fifo output destination.
 * @param string the null-terminated string to append.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
bool fifoPrintString(Fifo *fifo, const char *string);

/** Appends a cross platform line feed (CR-LF) which works on UNIX and Windows platforms.
 * @param fifo output destination.
 * @return true if there was sufficient space to store the result in fifo, false otherwise.
 */
bool fifoPrintLn(Fifo *fifo);

/** Appends a string and a (CRLF) line feed.
 * @param fifo output destination.
 * @param string the null-terminated string to append.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
inline static bool fifoPrintStringLn(Fifo *fifo, const char *string) {
	return fifoPrintString(fifo,string) && fifoPrintLn(fifo);
}

/** Appends a string and a line feed.
 * @param fifo output destination.
 * @param string the null-terminated string to append.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
inline static bool fifoPrintStringLf(Fifo *fifo, const char *string) {
	return fifoPrintString(fifo,string) && fifoPrintLf(fifo);
}

/** Append a given number of characters at the end of the current buffer contents.
 * @param fifo output destination.
 * @param chars an array of chars to append.
 * @param n the number of chars to append.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
bool fifoPrintChars(Fifo *fifo, const char *chars, unsigned n);


inline static bool fifoPrintBool(Fifo *fifo, bool bo) {
	return fifoPrintString(fifo, bo ? "true" : "false");
}

/** Prints a single BaseN digit,
 * @param fifo output destination.
 * @param value the number fitting into one digit of baseN, i.e. value <N.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
bool fifoPrintBaseNChar (Fifo *fifo, int value);

/** Format a number in hexadecimal notation.
 * @param fifo output destination.
 * @param value unsigned number.
 * @param minWidth the minimum number of output digits. Specifying minWidth=0 results in 0 being fifoPrinted as empty
 *   string. Numbers are padded with zeros.
 * @param maxWidth the maximum number of output digits. Large numbers are truncated at the left side. minWidth has
 *   precedence over maxWidth - specifying maxWidth=0 (generally any number<minWidth) results in output of exactly
 *   minWidth digits.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
bool fifoPrintHex(Fifo *fifo, unsigned value, int minWidth, int maxWidth);

/** Format a number in binary notation.
 * @param fifo output destination.
 * @param value unsigned number.
 * @param minWidth the minimum number of output digits. Specifying minWidth=0 results in 0 being fifoPrinted as empty
 *   string. Numbers are padded with zeros.
 * @param maxWidth the maximum number of output digits. Large numbers are truncated at the left side. minWidth has
 *   precedence over maxWidth - specifying maxWidth=0 (generally any number<minWidth) results in output of exactly
 *   minWidth digits.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
bool fifoPrintBin(Fifo *fifo, unsigned value, int minWidth, int maxWidth);

/** Format a number in binary notation, groups of digits separated by commas to improve legibility.
 * @param fifo output destination.
 * @param value unsigned number.
 * @param width the exact number of output digits. Optional commas 
 * @param groupSize the number of digits (counting from the LSB) until the next comma.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
bool fifoPrintBinFixed(Fifo *fifo, unsigned value, int width, int groupSize);

/** Prints a hex-dump like string of data.
 * @param fifo output destination
 * @param data the memory contents to display
 * @param dataSize the total size (bytes) of the data.
 * @param width the number of bytes per block. This is 1 for single bytes, 2 for 16-bit ints. The order of the bytes is
 *   reversed within a block to get readable hex numbers, with leading MSB. 
 */
bool fifoPrintHexString(Fifo *fifo, const char *data, int dataSize, int width);

/** Format a number in base-adic notation. Digits larger than 9 are represented by uppercase letters starting at 'A'
 *   (10). This function is less efficient than fifoPrintHex for hexadecimal numbers.
 * @param fifo output destination.
 * @param value unsigned number.
 * @param minWidth the minimum number of output digits. Specifying minWidth=0 results in 0 being fifoPrinted as empty
 *   string. Numbers are padded with zeros.
 * @param maxWidth the maximum number of output digits. Large numbers are truncated at the left side. minWidth has
 *   precedence over maxWidth - specifying maxWidth=0 (generally any number<minWidth) results in output of exactly
 *   minWidth digits.
 * @param base 2 for binary numbers, 8 for ocatel numbers, 10 for (unsigned) decimal numbers, 16 for hexadecimal numbers, etc.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
bool fifoPrintBaseN(Fifo *fifo, unsigned value, unsigned minWidth, unsigned maxWidth, int base);

/** Format a number in base-adic notation. Digits larger than 9 are represented by uppercase letters starting at 'A'
 *   (10). This function is less efficient than fifoPrintHex for hexadecimal numbers.
 * @param fifo output destination.
 * @param value unsigned number.
 * @param minWidth the minimum number of output digits. Specifying minWidth=0 results in 0 being fifoPrinted as empty
 *   string. Numbers are padded with zeros.
 * @param maxWidth the maximum number of output digits. Large numbers are truncated at the left side. minWidth has
 *   precedence over maxWidth - specifying maxWidth=0 (generally any number<minWidth) results in output of exactly
 *   minWidth digits.
 * @param base 2 for binary numbers, 8 for ocatel numbers, 10 for (unsigned) decimal numbers, 16 for hexadecimal numbers, etc.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
bool fifoPrintBaseN64(Fifo *fifo, Uint64 value, unsigned minWidth, unsigned maxWidth, int base);

/** Format a fractional number in base-adic notation. Digits larger than 9 are represented by uppercase letters
 * starting at 'A' (10). This function is less efficient than fifoPrintHex for hexadecimal numbers.
 * @param fifo output destination.
 * @param value unsigned number.
 * @param scale unsigned number that represents the 1.0 .
 * @param minWidth the minimum number of output digits. Specifying minWidth=0 results in 0 being fifoPrinted as empty
 *   string. Numbers are padded with zeros.
 * @param maxWidth the maximum number of output digits. Large numbers are truncated at the left side. minWidth has
 *   precedence over maxWidth - specifying maxWidth=0 (generally any number<minWidth) results in output of exactly
 *   minWidth digits.
 * @param fractionWidth the number of characters used for the dot and the digits after the dot.
 * @param base 2 for binary numbers, 8 for ocatel numbers, 10 for (unsigned) decimal numbers, 16 for hexadecimal numbers, etc.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
bool fifoPrintBaseNFraction(Fifo *fifo, unsigned value, unsigned scale, unsigned minWidth, unsigned maxWidth,
	unsigned fractionWidth, int base);


/** Format a non-negative decimal number in a human-friendly way. Leading zeros are omitted, but can be requested by
 *   using a largen minWidth value. 
 * @param fifo output destination.
 * @param value unsigned number.
 * @param minWidth the minimum number of output digits. Specifying minWidth=0 results in 0 being fifoPrinted as empty
 *   string. Numbers are padded with zeros.
 * @param maxWidth the maximum number of output digits. Large numbers are truncated at the left side. minWidth has
 *   precedence over maxWidth - specifying maxWidth=0 (generally any number<minWidth) results in output of exactly
 *   minWidth digits.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
bool fifoPrintUDec(Fifo *fifo, unsigned value, unsigned minWidth, unsigned maxWidth);

/** Format a non-negative decimal number in a human-friendly way. Leading zeros are omitted, but can be requested by
 *   using a largen minWidth value. 
 * @param fifo output destination.
 * @param value unsigned number.
 * @param minWidth the minimum number of output digits. Specifying minWidth=0 results in 0 being fifoPrinted as empty
 *   string. Numbers are padded with zeros.
 * @param maxWidth the maximum number of output digits. Large numbers are truncated at the left side. minWidth has
 *   precedence over maxWidth - specifying maxWidth=0 (generally any number<minWidth) results in output of exactly
 *   minWidth digits.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
bool fifoPrintUDec64(Fifo *fifo, Uint64 value, unsigned minWidth, unsigned maxWidth);


/** Format a signed decimal number in a human-friendly way. Leading zeros are omitted, but can be requested by
 *   using an appropriate minWidth value. Negative values are prefixed by a minus sign. The sign has precedence over digits
 *   if output digits are limited.
 * @param fifo output destination.
 * @param value signed number.
 * @param minWidth the minimum number of output digits. Specifying minWidth=0 results in 0 being fifoPrinted as empty
 *   string. Specifying minWidth=1 and showPositive=true results in 0 being printed as a single plus sign.Numbers are
 *   padded with zeros.
 * @param maxWidth the maximum number of output digits. Large numbers are truncated at the left side. minWidth has
 *   precedence over maxWidth - specifying maxWidth=0 (generally any number<minWidth) results in output of exactly
 *   minWidth digits.
 * @param showPositive prefix non-negative numbers with a plus sign.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
bool fifoPrintSDec(Fifo *fifo, int value, unsigned minWidth, unsigned maxWidth, bool showPositive);

/** Format a signed decimal number. Use shortest possible notation: no plus sign, no leading zeros.
 * @param fifo output destination.
 * @param value signed number.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
bool fifoPrintInt (Fifo *fifo, int value);

/** Format a signed decimal number in a human-friendly way. Leading zeros are omitted, but can be requested by
 *   using an appropriate minWidth value. Negative values are prefixed by a minus sign. The sign has precedence over digits
 *   if output digits are limited.
 * @param fifo output destination.
 * @param value signed number.
 * @param minWidth the minimum number of output digits. Specifying minWidth=0 results in 0 being fifoPrinted as empty
 *   string. Specifying minWidth=1 and showPositive=true results in 0 being printed as a single plus sign.Numbers are
 *   padded with zeros.
 * @param maxWidth the maximum number of output digits. Large numbers are truncated at the left side. minWidth has
 *   precedence over maxWidth - specifying maxWidth=0 (generally any number<minWidth) results in output of exactly
 *   minWidth digits.
 * @param showPositive prefix non-negative numbers with a plus sign.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
bool fifoPrintSDec64(Fifo *fifo, Int64 value, unsigned minWidth, unsigned maxWidth, bool showPositive);

/** Format a signed decimal fraction in a human-friendly way. Leading zeros are omitted, but can be requested by
 *   using an appropriate minWidth value. Negative values are prefixed by a minus sign. The sign has precedence over digits
 *   if output digits are limited. The decimal dot is printed unless the number of characters is too limited.
 *   The value printed is rounded.
 * @param fifo output destination.
 * @param value signed number.
 * @param scale signed number, that represents the value 1.0 .
 * @param minWidth the minimum number of integral output digits. Specifying minWidth=0 results in 0 being fifoPrinted as empty
 *   string. Specifying minWidth=1 and showPositive=true results in 0 being printed as a single plus sign. Numbers are
 *   padded with zeros.
 * @param maxWidth the maximum number of integral output digits. Large numbers are truncated at the left side. minWidth has
 *   precedence over maxWidth - specifying maxWidth=0 (generally any number<minWidth) results in output of exactly
 *   minWidth digits.
 * @param fracDigits the exact number of characters used for the dot and digits after the dot. Specifying 0 suppresses
 *   the dot.
 * @param showPositive prefix non-negative numbers with a plus sign.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 */
bool fifoPrintFixedPointInt(Fifo *fifo, int value, int scale, unsigned minWidth, unsigned maxWidth,
	unsigned fracDigits, bool showPositive);

/**
 * Format a signed decimal fraction in a human-friendly way. Leading zeros are omitted, but can be requested by
 *   using an appropriate minWidth value. Negative values are prefixed by a minus sign. The sign has precedence over digits
 *   if output digits are limited. The decimal dot is printed unless the number of characters is too limited.
 *   The value printed is rounded.
 * @param fifo output destination.
 * @param value signed number.
 * @param scale signed number, that represents the value 1.0 .
 * @param minWidth the minimum number of integral output digits. Specifying minWidth=0 results in 0 being fifoPrinted as empty
 *   string. Specifying minWidth=1 and showPositive=true results in 0 being printed as a single plus sign. Numbers are
 *   padded with zeros.
 * @param maxWidth the maximum number of integral output digits. Large numbers are truncated at the left side. minWidth has
 *   precedence over maxWidth - specifying maxWidth=0 (generally any number<minWidth) results in output of exactly
 *   minWidth digits.
 * @param fracDigits the exact number of characters used for the dot and digits after the dot. Specifying 0 suppresses
 *   the dot.
 * @param showPositive prefix non-negative numbers with a plus sign.
 * @return true, if there was sufficient space to store the result in fifo, false otherwise.
 * @deprecated Use fifoPrintFixedPointInt instead.
 */
inline static bool fifoPrintSDecFraction(Fifo *fifo, int value, int scale, unsigned minWidth, unsigned maxWidth,
	unsigned fracDigits, bool showPositive) {
	return fifoPrintFixedPointInt(fifo,value,scale,minWidth,maxWidth,fracDigits,showPositive);

}


/** Output a float value in human-readable technical form: sign, 3 or more digits and decimal dot plus a character
 * multiplier denoting the exponent. Example: 1000 is displayed as 10.k .
 * BUGS: Rounding is still lacking (+0.5*10^-singificandWidth or similar). Values larger than 999Y (Yotta, 1E+24) or smaller
 * than 1.y (yokto, 1E-24) cannot be displayed, although float extends to eponents up to 39.
 * @param fifo output destination
 * @param significandWidth must be at least 3. This is the length of the mantissa excluding the sign, but including the dot.
 * @param value the float value to display.
 * @return true, if fifo had no overrun, false otherwise.
 */
bool fifoPrintTechnicalFloat(Fifo *fifo, float value, int significandWidth);

bool fifoPrintHex32Masked(Fifo *fifo, unsigned value, unsigned mask, char dontCare);

bool fifoPrintHex64(Fifo *fifo, unsigned long long value, unsigned minWidth, unsigned maxWidth);
bool fifoPrintBin64(Fifo *fifo, unsigned long long value, unsigned minWidth, unsigned maxWidth);

/** Outputs the contents of a Fifo with translation of control characters into hex escapes.
 * @param output the output Fifo
 * @param fifo the Fifo to dump.
 * @return true if output capacity was sufficient, false otherwise.
 */
bool fifoDumpFifoAscii(Fifo *output, Fifo *fifo);

/** Outputs the contents of a Fifo with translation of control characters into hex escapes.
 * @param output the output Fifo
 * @param fifo the Fifo to dump.
 * @param bytesPerLine the number of bytes to output per line.
 * @param ascii show printable characters next to hex dump, separated by 3 spaces.
 * @return true if output capacity was sufficient, false otherwise.
 */
bool fifoDumpFifoHex(Fifo *output, Fifo *fifo, int bytesPerLine, bool ascii);

#endif
