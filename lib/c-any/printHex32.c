#include <print.h>

void printHex32(unsigned long n) {
	for (unsigned char i=0; i<32/4; i++, n <<= 4)
		printXDigit(n >> ((32/4-1)*4));
}
