#include <print.h>

void printUDec32(unsigned long n) {
	const unsigned long p10[] =
		{ 1000000000
		, 100000000
		, 10000000
		, 1000000
		, 100000
		, 10000
		, 1000
		, 100
		, 10
		, 1 };

	if (n==0) printDigit(0);
	else {
		int p;
		for (p=0; p10[p]>n; p++);
		for (; p<sizeof p10/sizeof(p10[0]); p++) {
			unsigned int d = n / p10[p];
			printDigit(d);
			n -= d*p10[p];
		}
	}
}

