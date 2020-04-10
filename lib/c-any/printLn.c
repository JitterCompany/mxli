#include <print.h>

NewlineStyle printNewlineStyle = NEWLINE_LF;

void printLn() {
	switch(printNewlineStyle) {
		case NEWLINE_LF:	printChar('\n'); break;
		case NEWLINE_CR:	printChar('\r'); break;
		default:		printChar('\r'); printChar('\n');
	}
}
