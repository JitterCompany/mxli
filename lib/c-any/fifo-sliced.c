//HEADER
/*
  fifo-sliced.c 
  Copyright 2006-2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <fifo.h>
#include <string.h>
#include <macros.h>

//SLICE
size_t fifoCanReadLinear(Fifo const *fifo) {
	const size_t r = fifoCanRead(fifo);
	const size_t l = fifo->size - fifo->rPos -1;
	return r<l ? r : l;
}

//SLICE
size_t fifoCanWriteLinear(Fifo const *fifo) {
	const size_t w = fifoCanWrite(fifo);
	const size_t l = fifo->size - fifo->wPos -1;
	return w<l ? w : l;
}

//SLICE
// :o) quick and dirty implementation. Thread safety not maximized.
bool fifoPutFifo(Fifo *fifo, Fifo *source) {
	if (fifoCanWrite(fifo) >= fifoCanRead(source)) while (fifoCanRead(source))
		if (fifoCanWrite(fifo)) fifoWrite(fifo,fifoRead(source));
		else return false;
	else return false;

	return true;
}

//SLICE
bool fifoMoveFifo (Fifo *fifo, Fifo *source, size_t n) {
	if (fifoCanWrite(fifo) >= n
	&& fifoCanRead(source) >= n) {
		while (n--) fifoWrite (fifo, fifoCanRead(source));
		return true;
	}
	else return false;
}

//SLICE
bool fifoExtract(Fifo *fifo, void (*print)(char), unsigned nMin, unsigned nMax) {
	if (nMin <= fifoCanRead(fifo)) {
		for (int i=0; i<nMax && fifoCanRead(fifo); ++i)
			if (print!=0) print(fifoRead(fifo));
			else fifoRead(fifo);		// just consume characters
		return true;
	}
	else return false;
}

//SLICE
void fifoFlush(Fifo *fifo, void (*_print)(char)) {
	fifoExtract(fifo, _print, 0, -1);
}

//SLICE
bool fifoGetUint16(Fifo *fifo, Uint16 *int16) {
	if (fifoCanRead(fifo)>=2) {
		const int lo = fifoRead(fifo);
		const int hi = fifoRead(fifo);
		*int16 = lo & 0xFF | 0xFF00 & hi<<8;
		return true;
	}
	else return false;
}

//SLICE
bool fifoGetnetUint16(Fifo *fifo, Uint16 *int16) {
	if (fifoCanRead(fifo)>=2) {
		const int hi = fifoRead(fifo);
		const int lo = fifoRead(fifo);
		*int16 = lo & 0xFF | 0xFF00 & hi<<8;
		return true;
	}
	else return false;
}

//SLICE
bool fifoGetUint32(Fifo *fifo, Uint32 *int32) {
	if (fifoCanRead(fifo)>=4) {
		int v = 0;
		for (int c=0; c<4; ++c) v = v | (0xFF & fifoRead(fifo)) << c*8;
		*int32 = v;
		return true;
	}
	else return false;
}

//SLICE
bool fifoGetnetUint32(Fifo *fifo, Uint32 *int32) {
	if (fifoCanRead(fifo)>=4) {
		int v = 0;
		for (int c=0; c<4; ++c) v = v<<8 | 0xFF & fifoRead(fifo);
		*int32 = v;
		return true;
	}
	else return false;
}

//SLICE
bool fifoPutInt16(Fifo *fifo, int value) {
	if (fifoCanWrite(fifo)>=2) {
		fifoWrite(fifo,value & 0xFF);
		fifoWrite(fifo,value>>8 & 0xFF);
		return true;
	}
	else return false;
}

//SLICE
bool fifoPutnetInt16(Fifo *fifo, int value) {
	if (fifoCanWrite(fifo)>=2) {
		fifoWrite(fifo,value>>8 & 0xFF);
		fifoWrite(fifo,value & 0xFF);
		return true;
	}
	else return false;
}

//SLICE
bool fifoPutInt32(Fifo *fifo, int value) {
	if (fifoCanWrite(fifo)>=2) {
		for (int c=0; c<4; ++c) fifoWrite(fifo,value>>8*c & 0xFF);
		return true;
	}
	else return false;
}

//SLICE
bool fifoPutnetInt32(Fifo *fifo, int value) {
	if (fifoCanWrite(fifo)>=2) {
		for (int c=0; c<4; ++c) fifoWrite(fifo,value>>(32-8*c) & 0xFF);
		return true;
	}
	else return false;
}

//SLICE
bool fifoToStringZ(Fifo *fifo, char *buffer, size_t bufferSize) {
	Fifo clone = *fifo;
	if (fifoCanRead(&clone)<bufferSize) {
		while (fifoCanRead(&clone)) *buffer++ = fifoRead(&clone);
		*buffer = 0;
		return true;
	}
	else return false;
}

