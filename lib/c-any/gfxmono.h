/*
  gfxmono.h 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef gfxmono_h
#define gfxmono_h

/** @file
 * @brief Monochrome graphics functions and basic font definitions.
 *
 */

#include <stdbool.h>
#include <geomInt.h>

/** Byte-oriented pixel drawing surface in memory upper left pixel is (0,0).
 */
typedef struct {
	unsigned nX;		///< width of surface
	unsigned nY;		///< height of surface
	unsigned unitX;		///< how many bits to move in buffer, if x increases by one.
	unsigned unitY;		///< how many bits to move in buffer, if y increases by one.
	void *pixels;		///< pixel buffer memory.
} Gfxmono;

/** Needed to define Fonts.
 */
typedef struct CharLocation {
	unsigned x;
	unsigned y;
	unsigned nX;
} CharLocation;

/** Basic Font definition. A font is a monochrome buffer (Gfxmono) that contains all character images of a font and a
 * function that that finds position and with of any character's (sub-) image within the buffer.
 */
typedef struct Font {
	const Gfxmono*	gfx;				///< all character images
	unsigned	nY;				///< height of the font (all chars)
	CharLocation	(*charLocation)(char c);	///< function for finding sub-images.
} Font;

/** Sets a single pixel if it lies within the valid area of an image buffer.
 * @param gfx destination gfx
 * @param x x-position of pixel
 * @param y y-position of pixel
 * @param color the color of the pixel (0=off, 1=on)
 */
void gfxmonoSetPixel(Gfxmono *gfx, int x, int y, bool color);

/** Sets a single pixel. No range checking is performed.
 * @param gfx destination gfx
 * @param x x-position of pixel
 * @param y y-position of pixel
 * @param color the color of the pixel (0=off, 1=on)
 */
void gfxmonoSetPixelFast(Gfxmono *gfx, int x, int y, bool color);

/** Reads a single pixel. 
 * @param gfx destination Gfxmono
 * @param x x-position of pixel
 * @param y y-position of pixel
 * @return true, if pixel was set and lies within image buffer, false otherwise
 */
bool gfxmonoGetPixel(const Gfxmono *gfx, int x, int y);

/** Reads a single pixel. No range checking is performed.
 * @param gfx destination Gfxmono
 * @param x x-position of pixel
 * @param y y-position of pixel
 * @return true, if pixel was set, false otherwise
 */
bool gfxmonoGetPixelFast(const Gfxmono *gfx, int x, int y);

/** Copy a rectangular area from src to dst. Range checking is performed. Writing outside the bounds of dst has no
 * effect. Reading outside the bounds of src results in reads of 0 (false).
 */
void gfxmonoBitBlt(
	Gfxmono *dstGfxmono, int dstX, int dstY, int nX, int nY, bool color,
	const Gfxmono *srcGfxmono, int srcX, int srcY);

void gfxmonoBitBltFill(Gfxmono *gfx, int x0, int y0, int nX, int nY, bool color);

/** Draws a single character.
 * @param gfx byte-oriented pixel bits gfx
 * @param x x position (pixels)
 * @param y y position (pixels)
 * @param font the font to use
 * @param c character to print
 * @param color true for pixel set, false for pixel cleared
 * @return width of the char
 */
unsigned gfxmonoDrawChar(Gfxmono *gfx, int x, int y, Font const *font, char c, bool color);


/** Draws a sequence of characters. Control characters are not translated into appropriate actions. Especially LF does
 * not cause an emulation of a line feed.
 * @return the width (pixels) of the sequence drawn.
 */
unsigned gfxmonoDrawString(Gfxmono *gfx, int x, int y, Font const *font, const char *string, bool color);

/** Defines the bounding box containing all pixels of a given area.
 */
typedef R2 BoundingBox;

/** Check if the given bounding box is empty
 */
bool emptyBoundingBox(const BoundingBox *boundingBox);

/** Calculate the smallest bounding box.
 * @param gfx image contents (pixels).
 * @param x left bound of image data.
 * @param y upper bound of image data.
 * @param nX image data width.
 * @param nY image data height.
 * @param color the color used for marking pixels.
 * @return the smallest bounding box (rectangle) that contains all pixels of color color.
 */
BoundingBox gfxmonoGetBoundingBox(const Gfxmono *gfx, int x, int y, int nX, int nY, bool color);

/** Calculate the bounding box of a char.
 * @param font the font to use.
 * @param c the character to use.
 * @return the smallest bounding box (rectangle) that contains all white pixels.
 */
BoundingBox fontGetBoundingBoxChar(Font const *font, char c);

/** Construct a charLocation function for a variable length font.
 * @param font a (most likely) monospaced font.
 * @param c the character to display.
 * @param blankSpace the number of columns, that should be assumed 'non-blank' in case of a blank character.
 * @param spacing the number of plain columns after a char. Typically this is 1.
 */
CharLocation fontCharLocationVariableLength(Font const *font, char c, int blankSpace, int spacing);

/** Sets a single pixel if it lies within the valid area of an image buffer.
 * @param gfx destination gfx
 * @param p position of pixel
 * @param color the color of the pixel (0=off, 1=on)
 */
void gfxmonoDrawPixel(Gfxmono *gfx, const P2 p, bool color);

/** Draws a line.
 * @param gfx a graphics surface to operate on
 * @param p0 starting point of line (included).
 * @param p1 end point of line (included).
 * @param color the drawing color
 */
void gfxmonoDrawLine(Gfxmono *gfx, const P2 p0, const P2 p1, bool color);

/** Draws a rectangular box.
 * @param gfx a graphics surface to operate on
 * @param r a rectangle defining the outline of the box
 * @param color the drawing color
 */
void gfxmonoDrawBox(Gfxmono *gfx, R2 const r, bool color);

/** Draws a chess pattern.
 * @param gfx a graphics surface to operate on.
 * @param window the area to fill.
 * @param w the width of the boxes.
 * @param color0 the color to use at coordinate (0,0).
 */
void gfxmonoChess(Gfxmono *gfx, const R2 window, int w, bool color0);

/** Scrolls the contents of the image up or down. New contents is cleared.
 * @param gfx a graphics surface to operate on.
 * @param window the area to scroll.
 * @param nUp the number of pixels to scroll up if positive, the number of pixels to scroll down if negative.
 * @param color the fill color for blank portions.
 */
void gfxmonoScrollY(Gfxmono *gfx, R2 const window, int nUp, bool color);

/** Scrolls the contents of the image left or right. New contents is cleared.
 * @param gfx a graphics surface to operate on.
 * @param window the area to scroll.
 * @param nLeft the number of pixels to scroll left if positive, the number of pixels to scroll right down if negative.
 * @param color the fill color for blank portions.
 */
void gfxmonoScrollX(Gfxmono *gfx, R2 const window, int nLeft, bool color);

#endif

