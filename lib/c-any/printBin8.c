#include <print.h>

void printBin8(unsigned char n) {
	for (signed char i=8-1; i>=0; i--) printDigit(n >> i &1);
}
