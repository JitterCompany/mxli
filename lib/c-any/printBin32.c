#include <print.h>

void printBin32(unsigned long n) {
	for (signed char i=32-1; i>=0; i--) printDigit(n >> i &1);
}
