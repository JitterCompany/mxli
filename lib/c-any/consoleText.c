/*
  consoleText.c 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <consoleText.h>
#include <fatal.h>

static const Font *consoleFont = 0;

static int (*consoleTabstop)(int n) = 0;

// BEWARE: the two functions consolePrintChar and weakConsolePrintChar must reside in the same compilation unit.
void consoleInterfaceInit(const Font *font, int (*tabstop)(int n)) {
	consoleFont = font;
	consoleTabstop = tabstop;
}

void consolePrintChar(char c) __attribute__((weak,alias("weakConsolePrintChar")));

static void weakConsolePrintChar(char c) {
	fatal("Called weak reference of consolePrintChar");
}

const Font* consoleGetFont(void) {
	return consoleFont;
}

int consoleGetTabStop(int n) {
	if (consoleTabstop!=0) return (*consoleTabstop)(n);
	else if (consoleFont==0) return 8*n;	// text mode: 8 chars
	else return 8*8*n;			// graphics mode: 8*8pixels
}

