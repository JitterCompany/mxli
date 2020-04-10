//HEADER
/*
  fifoq-sliced.c 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <string.h>
#include <fifoq.h>

//SLICE
bool fifoqPrintString(Fifoq* q, const char* s) {
	const size_t length = strlen(s);
	if (fifoCanWrite(q) > length) {
		fifoPrintString(q,s);
		return fifoqPrintNext(q);
	}
	else return false;
}

//SLICE
bool fifoqPrintFifo(Fifoq* q, Fifo *s) {
	if (fifoCanWrite(q) > fifoCanRead(s)) {
		fifoAppend(q,s);
		return fifoqPrintNext(q);
	}
	else return false;
}

//SLICE
bool fifoqParseSkipToNext(Fifoq* q) {
	Fifo clone = *q;
	while (fifoCanRead(&clone)) {
		const char c = fifoRead(&clone);
		if (c==FIFO_Q_LIST_SEPARATOR) {
			fifoCopyReadPosition(q,&clone);
			return true;
		}
	}
	return false;
}

//SLICE
size_t fifoqCount (const Fifoq* q) {
	Fifoq clone = *q;
	size_t n = 0;
	while (fifoqParseSkipToNext (&clone)) n++;

	return n;
}

