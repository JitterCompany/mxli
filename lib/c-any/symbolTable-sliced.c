//HEADER
/*
  symbolTable-sliced.c 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <symbolTable.h>

//SLICE
bool streq(const char *a, const char *b) {
	for ( ;*a!=0 && *b!=0; ++a, ++b) if (*a!=*b) return false;

	return true;
}

//SLICE
bool symbolLookUp(const SymbolAssoc symbolTable[], const char *symbol, int *value) {
	if (symbol==0 || symbol[0]==0) return false;
	for (int s=0; symbolTable[s].symbol!=0; ++s) if (streq(symbol,symbolTable[s].symbol)) {
		*value = symbolTable[s].value;
		return true;
	}
	return false;
}

