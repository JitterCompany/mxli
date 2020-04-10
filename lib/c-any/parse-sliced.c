//HEADER
#include <parse.h>
#include <string.h>

//SLICE
bool isSpace(char c) {
	switch(c) {
		case ' ':
		case '\t':
		case '\r':
		case '\n':
		case '\f': return true;
		default: return false;
	}
}

//SLICE
int baseNDigitToInt(char c) {
	if ('0'<=c && c<='9') return c-'0';
	else if ('A'<=c && c<='Z') return c-'A'+0xA;
	else return c-'a'+0xa;
}

