#include <fbConsole.h>
#include <fonts.h>

enum {
	N_X	= 16,
	N_Y	= 8,
};

static bool colorFg;	// text and drawing foreground color.

static const Font * const font = &fontVga8x8;
static struct {
	int x;
	int y;
} fbConsolePosition;

void fbConsoleInit(bool _colorFg) {
	colorFg = _colorFg;
	fbConsolePosition.x = 0;
	fbConsolePosition.y = 0;
}

void fbConsoleClear(void) {
	fb128x64Clear(!colorFg);	// clear to background color
}

int fbConsoleXyChar(int x, int y, char c) {
	const int gx = fbConsoleX(x);
	const int gy = fbConsoleY(y);
	gfxmonoDrawChar(&fb128x64,gx,gy,font,c,colorFg);
	return x+1;
}

int fbConsoleXyString(int x, int y, const char* text) {

	while (*text!=0) fbConsoleXyChar(x++,y,*text++);

	return x;
}

int fbConsoleXyFifo(int x, int y, Fifo *fifo) {

	while (fifoCanRead(fifo)) fbConsoleXyChar(x++,y,fifoRead(fifo));

	return x;
}

void fbConsoleXClear(int x, int y, int xTo) {
	while (x<xTo) fbConsoleXyChar(x++,y,' ');
}

void fbConsoleEolClear(int x, int y) {
	while (x<N_X) fbConsoleXyChar(x++,y,' ');
}

void fbConsoleScrollY(int nY) {
	fb128x64ScrollY(nY*8,!colorFg);
}

void fbConsolePrintNewLine(void) {
	if (fbConsolePosition.y+1<N_Y) {
		fbConsolePosition.y ++;
		fbConsolePosition.x = 0;
	}
	else {	// scroll contents up
		fbConsoleScrollY(-1);
		fbConsolePosition.x = 0;
		fbConsoleEolClear(fbConsolePosition.x, fbConsolePosition.y);
	}
}

void fbConsolePrintChar(char c) {
	const int x = fbConsolePosition.x;

	switch(c) {
		case '\x03':	fbConsoleEolClear (fbConsolePosition.x, fbConsolePosition.y);
				fbConsolePosition.x = x;
				break;
		case '\f':	fbConsolePosition.x = 0;
				fbConsolePosition.y = 0;
				fbConsoleClear();
				break;
		case '\n':	fbConsolePrintNewLine();
				break;
		case '\r':	fbConsolePosition.x = 0;
				break;
		default:	if (c & 0x80) {	// special controls
					if ((c & 0x7F) < 16) fbConsolePosition.x = c & 0x7F;
					else if ((c & 0x7F) < 32) fbConsolePosition.y = (c & 0x7F) - 16;
					else /* WTF? */;
				}
				else {	// normal character
					if (fbConsolePosition.x>=N_X) fbConsolePrintNewLine();
					fbConsoleXyChar(fbConsolePosition.x++,fbConsolePosition.y,c);
					break;
				}
	}
}

void fbConsolePrintString(const char *text) {
	while (*text) fbConsolePrintChar(*text++);
}

void fbConsolePrintFifo(Fifo *fifo) {
	while (fifoCanRead(fifo)) fbConsolePrintChar(fifoRead(fifo));
}

