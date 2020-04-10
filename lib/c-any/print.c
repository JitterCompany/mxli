#include <print.h>

void (*printCharPointer)(char) = 0;

void printChar(char c) {
	if (printCharPointer) (*printCharPointer)(c);
}

void print(void (*pc)(char)) {
	printCharPointer = pc;
}

