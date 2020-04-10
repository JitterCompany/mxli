#include <print.h>

void printSDec64(signed long long n) {
	if (n<0) {
		printChar('-');
		n = -n;
	}
	printUDec64(n);
}

