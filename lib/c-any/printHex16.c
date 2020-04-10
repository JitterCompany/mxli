#include <print.h>

void printHex16(unsigned short n) {
	for (unsigned char i=0; i<16/4; i++, n <<= 4)
		printXDigit(n >> ((16/4-1)*4));
}
