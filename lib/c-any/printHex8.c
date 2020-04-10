#include <print.h>

void printHex8(unsigned char n) {
	for (unsigned char i=0; i<8/4; i++, n <<= 4)
		printXDigit(n >> ((8/4-1)*4));
}
