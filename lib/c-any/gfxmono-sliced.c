//HEADER
/*
  gfxmono-sliced.c
  Copyright 2011,2017 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <gfxmono.h>
#include <integers.h>	//replaces: #include <limits.h>


//SLICE
void gfxmonoSetPixel(Gfxmono *gfx, int x, int y, bool color) {
	if (0<=x && x<gfx->nX && 0<=y && y<gfx->nY) gfxmonoSetPixelFast(gfx,x,y,color);
}

//SLICE
bool gfxmonoGetPixel(const Gfxmono *gfx, int x, int y) {
	return
		(0<=x && x<gfx->nX && 0<=y && y<gfx->nY)
		&& gfxmonoGetPixelFast(gfx,x,y);
}
		
//SLICE
void gfxmonoSetPixelFast(Gfxmono *gfx, int x, int y, bool color) {
	const unsigned bitNo = x*gfx->unitX + y*gfx->unitY;
	const unsigned byteNo = bitNo / 8;
	unsigned bitInByte = bitNo % 8;

	((unsigned char*)gfx->pixels)[byteNo] =
	((const unsigned char*)gfx->pixels)[byteNo] & ~(1<<bitInByte) | (unsigned char)color<<bitInByte;
}


//SLICE
bool gfxmonoGetPixelFast(const Gfxmono *gfx, int x, int y) {
	const unsigned bitNo = x*gfx->unitX + y*gfx->unitY;
	const unsigned byteNo = bitNo / 8;
	unsigned bitInByte = bitNo % 8;

	return (bool) ( ((const unsigned char*)gfx->pixels)[byteNo] >> bitInByte & 1);
}


//SLICE
void gfxmonoBitBlt(
	Gfxmono *dstGfxmono, int dstX, int dstY, int nX, int nY, bool color,
	const Gfxmono *srcGfxmono, int srcX, int srcY) {

	for (int dx=0; dx<nX; dx++) for (int dy=0; dy<nY; dy++) {
		const bool pixel = !color ^ ((0<=srcX+dx && srcX+dx<srcGfxmono->nX && 0<=srcY+dy && srcY+dy<srcGfxmono->nY) ?
				gfxmonoGetPixelFast(srcGfxmono,srcX+dx,srcY+dy) : false);


		if (0<=dstX+dx && dstX+dx<dstGfxmono->nX && 0<=dstY+dy && dstY+dy<dstGfxmono->nY)
			gfxmonoSetPixelFast(dstGfxmono,dstX+dx,dstY+dy,pixel);
	}
}


//SLICE
void gfxmonoBitBltFill(Gfxmono *gfx, int x, int y, int nX, int nY, bool color) {
	if (x<0) x = 0;
	if (y<0) y = 0;
	if (x+nX>gfx->nX) nX = gfx->nX - x;
	if (y+nY>gfx->nY) nY = gfx->nY - y;

	for (int dx=0; dx<nX; dx++) for (int dy=0; dy<nY; dy++) gfxmonoSetPixelFast(gfx,x+dx,y+dy,color);
}


//SLICE
unsigned gfxmonoDrawChar(Gfxmono *gfx, int x, int y, Font const *font, char c, bool color) {

	const CharLocation location = font->charLocation(c);

	gfxmonoBitBlt(
		gfx, x,y, location.nX, font->nY, color,
		font->gfx, location.x, location.y
	);

	return location.nX;
}

//SLICE
unsigned gfxmonoDrawString(Gfxmono *gfx, int x, int y, Font const *font, const char *string, bool color) {
	unsigned offset = 0;
	for ( ;*string; string++) {
		offset += gfxmonoDrawChar(gfx, x+offset,y, font, *string, color);
	}
	return offset;
}

//SLICE
BoundingBox gfxmonoGetBoundingBox(const Gfxmono *gfx, int x, int y, int nX, int nY, bool color) {
	BoundingBox boundingBox = {{
		INT32_MAX,		// minX
		INT32_MAX,		// minY
		INT32_MIN,		// maxX
		INT32_MIN		// maxY
	}};

	for (int dx=0; dx<nX; dx++) for (int dy=0; dy<nY; dy++) {
		if (gfxmonoGetPixel(gfx,x+dx,y+dy)==color) {
			if (x+dx<boundingBox.x0) boundingBox.x0 = x+dx;
			if (x+dx>boundingBox.x1) boundingBox.x1 = x+dx;
			if (y+dy<boundingBox.y0) boundingBox.y0 = y+dy;
			if (y+dy>boundingBox.y1) boundingBox.y1 = y+dy;
		}
	}
	return boundingBox;
}

//SLICE
bool emptyBoundingBox(const BoundingBox *boundingBox) {
	return boundingBox->x0==INT32_MAX;
}

//SLICE
BoundingBox fontGetBoundingBoxChar(const Font *font, char c) {

	const CharLocation charLocation = font->charLocation(c);

	BoundingBox boundingBox = gfxmonoGetBoundingBox(
		font->gfx,charLocation.x, charLocation.y, charLocation.nX, font->nY,true);

	if (!emptyBoundingBox(&boundingBox)) {
		boundingBox.x0 -= charLocation.x;
		boundingBox.x1 -= charLocation.x;
		boundingBox.y0 -= charLocation.y;
		boundingBox.y1 -= charLocation.y;
	}
	return boundingBox;
}

//SLICE
CharLocation fontCharLocationVariableLength(const Font *font, char c, int blankSize, int spacing) {
	const BoundingBox boundingBox = fontGetBoundingBoxChar(font,c);
	CharLocation charLocation = font->charLocation(c);

	if (!emptyBoundingBox(&boundingBox)) {
		CharLocation charLocationNew = {
			charLocation.x + boundingBox.x0,
			charLocation.y,
			boundingBox.x1-boundingBox.x0+1+spacing
		}; 
		return charLocationNew;
	}
	else {
		if (blankSize<charLocation.nX) charLocation.nX = blankSize;
		return charLocation;
	}
}


/*
void fbChess(int w) {
	Uint64 pattern = 0;
	for (int i=0; i<64; ++i) if (w && (i/w) & 1) pattern |= 1llu<<i;
	for (int x=0; x<128; ++x) graphicsBuffer[x] = w && x/w & 1 ? pattern : ~pattern;
}

void fbScrollY(int nY) {
	for (int x=0; x<128; ++x) {
		if (nY>=0) graphicsBuffer[x] = graphicsBuffer[x] << nY;
		else graphicsBuffer[x] = graphicsBuffer[x] >> -nY;
	}
}

void fbScrollX(int nX) {
	if (nX>=0) for (int x=127; x>=0; --x) graphicsBuffer[x] = x-nX >= 0 ? graphicsBuffer[x-nX] : 0;
	else for (int x=0; x<128; ++x) graphicsBuffer[x] = x-nX < 128 ? graphicsBuffer[x-nX] : 0;
}
*/
//SLICE
void gfxmonoDrawPixel(Gfxmono *gfx, const P2 p, bool color) {
	gfxmonoSetPixel(gfx,p.x,p.y,color);
}

//SLICE
void gfxmonoDrawLine(Gfxmono *gfx, const P2 p0, const P2 p1, bool color) {
	const P2 delta = p2Sub(p1,p0);
	for (P2 p={{0,0}}; !p2Eq(p,delta); p=p2Raster(delta,p)) {
		P2 pAbs = p2Add(p0,p);
		gfxmonoSetPixel(gfx,pAbs.x,pAbs.y,color);
	}
	gfxmonoSetPixel(gfx,p1.x,p1.y,color);
}

//SLICE
void gfxmonoDrawBox(Gfxmono *gfx, R2 const r, bool color) {
	const P2 p2 = {{ r.x0, r.y1 }};
	const P2 p3 = {{ r.x1, r.y0 }};
	gfxmonoDrawLine(gfx,r.ps[0],p2,color);
	gfxmonoDrawLine(gfx,p2,r.ps[1],color);
	gfxmonoDrawLine(gfx,r.ps[1],p3,color);
	gfxmonoDrawLine(gfx,p3,r.ps[0],color);
}

//SLICE
// efficient implementation without division.
void gfxmonoChess(Gfxmono *gfx, const R2 window, int w, bool color) {
	const R2 rGfx = {{ 0,0, gfx->nX, gfx->nY }};
	const R2 area = r2Intersect(rGfx,window);

	bool cx = color;
	for (int x=area.x0; x<=area.x1; cx = !cx) for (int xx=0; xx<w && x<=area.x1; ++xx,++x) {
		bool cy = false;
		for (int y=area.y0; y<=area.y1; cy = !cy) for (int yy=0; yy<w && y<=area.y1; ++yy,++y) {
			gfxmonoSetPixel(gfx,x,y, cx^cy);
		}
	}
}

void gfxmonoScrollY(Gfxmono *gfx, const R2 window, int nUp, bool color) {
	const R2 rGfx = {{ 0,0, gfx->nX, gfx->nY }};
	const R2 dest = r2Intersect(rGfx,window);

	if (nUp>0) {	// scroll up
		for (int y=dest.y0; y<=dest.y1; ++y) for (int x=dest.x0; x<=dest.x1; ++x)
			gfxmonoSetPixelFast(gfx,x,y, y+nUp<=dest.y1 ? gfxmonoGetPixelFast(gfx,x,y+nUp) : color);
	}
	else {		// scroll down
		for (int y=dest.y1; y>=dest.y0; --y) for (int x=dest.x0; x<=dest.x1; ++x)
			gfxmonoSetPixelFast(gfx,x,y, dest.y0<=y+nUp ? gfxmonoGetPixelFast(gfx,x,y+nUp) : color);

	}
}

void gfxmonoScrollX(Gfxmono *gfx, const R2 window, int nLeft, bool color) {
	const R2 rGfx = {{ 0,0, gfx->nX, gfx->nY }};
	const R2 area = r2Intersect(rGfx,window);
	if (nLeft>0)	// scroll left
		for (int x=area.x0; x<=area.x1; ++x) for (int y=area.y0; y<=area.y1; ++y)
			gfxmonoSetPixelFast(gfx,x,y, x+nLeft<=area.x1 ? gfxmonoGetPixelFast(gfx,x+nLeft,y) : color);
	else		// scroll right
		for (int x=area.x1; x>=area.x0; --x) for (int y=area.y0; y<=area.y1; ++y)
			gfxmonoSetPixelFast(gfx,x,y, area.x0<=x+nLeft ? gfxmonoGetPixelFast(gfx,x+nLeft,y) : color);

}
