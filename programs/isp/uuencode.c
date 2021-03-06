#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <fifoParse.h>
#include <fifoPrint.h>
#include "uu.h"

static char iBuffer[0x1000];
static char oBuffer[0x1000];

Fifo iFifo = { iBuffer, sizeof iBuffer };
Fifo oFifo = { oBuffer, sizeof oBuffer };

int main(void) {
	char c;

	while (EOF!=(c = getchar())) {
		fifoPrintChar(&iFifo,c);
	}

	while (fifoCanRead(&iFifo)) {
		fifoUuEncodeLine(&oFifo,&iFifo,0);
	}

	while (fifoCanRead(&oFifo)) putchar(fifoRead(&oFifo));

	return 0;
}
