#include <print.h>

void printHex64(unsigned long long n) {
	for (unsigned char i=0; i<64/4; i++, n <<= 4)
		printXDigit(n >> ((64/4-1)*4));
}
