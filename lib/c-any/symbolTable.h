/*
  symbolTable.h 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __symbolTable_h
#define __symbolTable_h

/** @file
 * @brief Mapping symbols to integer values.
 */

#include <stdbool.h>

/** Symbol-value-pair to create symbol tables.
 */
typedef struct {
	const char *	symbol;		///< symbol, 0 at end of list
	int		value;		///< numeric constant
} SymbolAssoc;

/** Simple string compare.
 * @param a first string.
 * @param b second string.
 * @return true, if a and b are equal, false otherwise.
 */
bool streq(const char *a, const char *b);

/** Looks up the (numeric) value of a symbol.
 * @param symbolTable a { 0 } - terminated array of symbol-value pairs.
 * @param symbol the symbol to look for
 * @param value a pointer to the location where to store the result in case of success.
 * @return true if symbol is found, false otherwise (and value unchanged).
 */
bool symbolLookUp(const SymbolAssoc symbolTable[], const char *symbol, int *value);

#endif
