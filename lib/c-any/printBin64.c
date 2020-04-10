#include <print.h>

void printBin64(unsigned long long n) {
	for (signed char i=64-1; i>=0; i--) printDigit(n >> i &1);
}
