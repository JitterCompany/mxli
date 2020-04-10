#include <print.h>

void printSDec32(signed long n) {
	if (n<0) {
		printChar('-');
		n = 1 + ~n;
	}
	printUDec32(n);
}

