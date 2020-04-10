#include <print.h>

void printBin16(unsigned short n) {
	for (signed char i=16-1; i>=0; i--) printDigit(n >> i &1);
}
