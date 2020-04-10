#include <print.h>

void printHexdump8(const void *data, int nBytes) {
	const unsigned char *byteData = (const unsigned char*)data;

	for (int b=0; b<nBytes; ++b) {
		if (b%16==0) printLn();
		else if (b%4==0) printChar(' ');
		printHex8(byteData[b]);
		printChar(' ');	
	}
	printLn();
}

