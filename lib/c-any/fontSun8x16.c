/* Linux kernel VGA8x8 font */

#include <fonts.h>

#include "fontSun8x16.inc"

static const Gfxmono imageBuffer = {
	8, 256*16, 1,8,  (void*) fontData
};

static CharLocation charLocation(char c) {
	const CharLocation charLocation = { 0, c*16, 8 };

	return charLocation;
}

static CharLocation charLocationV(char c) {
	return fontCharLocationVariableLength(&fontSun8x16,c,4,1);
}

const Font fontSun8x16 = { &imageBuffer, 16, &charLocation };
const Font fontSun8x16V = { &imageBuffer, 16, &charLocationV };

