#include <print.h>
#include <int64Math.h>

void printUDec64(unsigned long long n) {
	// 2**64 = 1.8E19
	const unsigned long long p10[] =
		{ 10000000000000000000llu	// 19 zeros
		, 1000000000000000000llu
		, 100000000000000000llu
		, 10000000000000000llu
		, 1000000000000000llu
		, 100000000000000llu
		, 10000000000000llu
		, 1000000000000llu
		, 100000000000llu
		, 10000000000llu
		, 1000000000llu
		, 100000000llu
		, 10000000llu
		, 1000000llu
		, 100000llu
		, 10000llu
		, 1000llu
		, 100llu
		, 10llu
		, 1llu };

	if (n==0) printDigit(0);
	else {
		int p;
		for (p=0; p10[p]>n; p++);
		for (; p<sizeof p10/sizeof(p10[0]); p++) {
			unsigned int d = uint64Div(n,p10[p]);
			printDigit(d);
			n -= d*p10[p];
		}
	}
}

