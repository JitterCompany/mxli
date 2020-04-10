// (C) Marc Prager, 2005
#ifndef PRINT_H
#define PRINT_H

/** @file
 * @brief Direct text output for a single, selectable channel.
 */

#include <newline.h>

#define printDigit(d) printChar((d)+'0')
#define printXDigit(d) printChar((d)<0xA ? (d)+'0' : (d)-0xA+'A')

void print(void (*)(char c));
extern void (*printCharPointer)(char);

void printChar(char c);

/** Determines how a newline is printed
 */
extern NewlineStyle printNewlineStyle;

/** Print a new line char or char sequence.
 * 
 */
void printLn();

void printString(const char *s);
void printStringLn (const char *s);

void printHex64(unsigned long long n);
void printHex32(unsigned long n);
void printHex16(unsigned short n);
void printHex8(unsigned char n);

void printBin64(unsigned long long n);
void printBin32(unsigned long n);
void printBin16(unsigned short n);
void printBin8(unsigned char n);

void printUDec64(unsigned long long n);
void printSDec64(signed long long n);
void printUDec32(unsigned long n);
void printSDec32(signed long n);

void printHexdump8(const void *data, int nBytes);
void printHexdump16(const void *data, int nBytes);
void printHexdump32(const void *data, int nBytes);

void printFixedPointInt(int v, int base, int afterComma);
#endif
