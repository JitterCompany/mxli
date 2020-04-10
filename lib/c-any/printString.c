#include <print.h>

void printString(const char *s) {
	while(*s) printChar(*s++);
}

void printStringLn (const char *s) {
	printString (s);
	printLn ();
}

