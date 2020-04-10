#include <print.h>

void printHexdump32(const void *data, int nBytes) {
	const unsigned long *wordData = (const unsigned long*)data;

	for (int w=0; w<nBytes/4; ++w) {
		if (w%8==0) printLn();
		else if (w%4==0) printChar(' ');
		printHex32(wordData[w]);
		printChar(' ');	
	}
	printLn();
}

