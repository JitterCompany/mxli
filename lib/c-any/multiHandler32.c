/*
  multiHandler32.c 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <multiHandler32.h>

void multiHandler32ListInit(MultiHandler32List list) {
	list[0].handler = 0;
}

bool multiHandler32ListAdd(MultiHandler32List list, void (*handler)(Uint32), Uint32 mask, int nList) {
	for (int h=0; h<nList-1; h++) {
		if (list[h].handler==0) {
			list[h].handler = handler;
			list[h].ir = mask;
			list[h+1].handler = 0;
			return true;
		}
	}
	return false;
}

void multiHandler32Dispatch(MultiHandler32List list, Uint32 ir) {
	for (int h=0; list[h].handler!=0; h++) {
		Uint32 irBits = ir & list[h].ir;
		if (irBits!=0) (*list[h].handler)(irBits);
	}
}

