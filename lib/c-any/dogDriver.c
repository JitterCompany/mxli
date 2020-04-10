/*
  dogDriver.c 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <dogDriver.h>

struct {
	DogDriverElement	*elements;
	int			n;
	int			wIdx;
	volatile int		rIdx;
	int			uIdx;		///< untouched limit
	int			index;
	int			subIndex;
	DogDriverElement const*	activeElement;
	volatile bool		isrIsActive;	///< ISR handler alive flag
} dogDriver = { };

static int ilog2(unsigned v) {
	for (int i=31; i>=0; --i) if (v & 1<<i) return i;
	return -1;
}

bool dogDriverIsActive(void) {
	return dogDriver.isrIsActive;
}

void dogDriverWillBeActivated(void) {
	dogDriver.isrIsActive = true;
}

void dogDriverSleep(void) {
	dogDriver.isrIsActive = false;
}

void dogDriverInit(DogDriverElement *elements, int n) {
	dogDriver.elements = elements;
	dogDriver.n = 1<<ilog2(n/sizeof(DogDriverElement));
	dogDriver.wIdx = 0;
	dogDriver.rIdx = 0;
	dogDriver.uIdx = 0;
	dogDriver.isrIsActive = false;
}

int dogDriverCanWrite(void) {
	return dogDriver.n - dogDriver.wIdx + dogDriver.rIdx;
}

bool dogDriverCanRead(void) {
	return dogDriver.wIdx > dogDriver.rIdx;
}

DogByte dogDriverReadByte(void) {
	if (dogDriver.rIdx==dogDriver.uIdx) {	// start next transfer
		dogDriver.uIdx++;
		dogDriver.activeElement = & dogDriver.elements[dogDriver.rIdx & dogDriver.n-1];
		dogDriver.index = 0;
		dogDriver.subIndex = dogDriver.activeElement->subIndex;
	}
	const DogByte db = dogDriver.activeElement->callback(
		dogDriver.activeElement->data, dogDriver.index,
		dogDriver.activeElement->n, dogDriver.subIndex
	);

	if (db & DOG_PREFIX) {
		dogDriver.subIndex++;
	}
	else {
		dogDriver.subIndex = dogDriver.subIndex & 0xFFFF0000;
		dogDriver.index++;
		if (dogDriver.index>=dogDriver.activeElement->n) dogDriver.rIdx++;
	}

	return db;
}

int dogDriverTransferNew(const void* data, int n, DogDriverCallback *callback, int subIndex) {
	if (!dogDriverCanWrite()) return 0;

	for (int i=dogDriver.uIdx; i<dogDriver.wIdx; ++i)
		if (dogDriver.elements[i & dogDriver.n-1].callback==callback) return i;
	return dogDriverTransfer(data,n,callback,subIndex) | 1<<31;
}

int dogDriverTransfer(const void* data, int n, DogDriverCallback *callback, int subIndex) {
	if (!dogDriverCanWrite()) return 0;

	DogDriverElement *e = & dogDriver.elements[dogDriver.wIdx & dogDriver.n-1];
	e->callback = callback;
	e->subIndex = subIndex;
	e->data = (const char*)data;
	e->n = n;
	return dogDriver.wIdx++ | 1<<31;
}

bool dogDriverTransferCompleted(int tid) {
	return tid < (dogDriver.rIdx | 1<<31);
}

void dogDriverWaitFor(int tid) {
	while (!dogDriverTransferCompleted(tid)) ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//

//SLICE
static DogByte callbackWriteCommand(const char *data, int index, int n, int subIndex) {
	return DOG_CMD | data[index];
}

int dogDriverWriteCommand(const char* data, int n) {
	return dogDriverStartTransfer(data,n,&callbackWriteCommand,0);
}

//SLICE
static DogByte callbackWriteData(const char *data, int index, int n, int subIndex) {
	return DOG_DATA | data[index];
}

int dogDriverWriteData(const char* data, int n) {
	return dogDriverStartTransfer(data,n,&callbackWriteData,0);
}



