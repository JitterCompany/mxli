/*
  geomInt.h 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef geomInt_h
#define geomInt_h

/** @file
 * @brief Basic mathematical functions on int-vectors (2D,3D).
 */

#include <stdbool.h>

/** 2-dimensional point.
 */
typedef union {
	struct	{	int	x,y; };
			int	vs[2];
} P2;

/** Constructor.
 */
inline static P2 p2(int x, int y) {
	const P2 p = {{ x,y }};
	return p;
}


/** 3-dimensional point.
 */
typedef struct {
	struct {	int	x,y,z; };
			int	vs[3];
} P3;

/** 2-dimensional rectangle.
 * Constraints: x0<=x1, y0<=y1, otherwise some functions will fail.
 */
typedef union {
	struct { 	int	x0,y0,x1,y1; };
			P2	ps[2];
			int	vs[4];
} R2;

/** Constructor.
 */
static inline R2 r2(int x0, int y0, int x1, int y1) {
	const R2 r = {{ x0,y0, x1,y1 }};
	return r;
}

/** 3-dimensional rectangle.
 */
typedef union {
	struct {	int	x0,y0,z0,x1,y1,z1; };
			P2	ps[3];
			int	vs[6];
} R3;


static inline int p2Mul(const P2 a, const P2 b) {
	return a.x*b.x + a.y*b.y;
}

static inline int p2VMul(const P2 a, const P2 b) {
	return a.x*b.y - a.y*b.x;
}

static inline int p2Abs2(const P2 p) {
	return p2Mul(p,p);
}

P2 p2Project(const P2 d, const P2 v);

static inline bool p2Eq(const P2 a, const P2 b) {
	return a.x==b.x && a.y==b.y;
}

static inline P2 p2Add(const P2 a, const P2 b) {
	const P2 amb = {{ a.x + b.x, a.y + b.y }};
	return amb;
}

static inline P2 p2Sub(const P2 a, const P2 b) {
	const P2 amb = {{ a.x - b.x, a.y - b.y }};
	return amb;
}

P2 p2Quadrant(const P2 p);

static inline P2 p2AddX(const P2 a, const P2 b) {
	const P2 r = {{ a.x+b.x, a.y }};
	return r;
}

static inline P2 p2AddY(const P2 a, const P2 b) {
	const P2 r = {{ a.x, a.y+b.y }};
	return r;
}

// direction, position
P2 p2Raster(const P2 d, const P2 p);

/** Sorts the x and the y-coordinates in order to satify the constraints mentioned at the definition of R2.
 * BEWARE: this function may change the rectangle, depending on your
 * interpretation of the bounds (including/excluding).
 */
R2 r2Normalize(const R2 r);

/** Calculates the intersection of 2 rectangles, which is again a rectangle.
 */
R2 r2Intersect(const R2 a, const R2 b);

/** Calculates the smallest Rectangle, that includes both rectangles passed.
 */
R2 r2BoundingRectangle(const R2 a, const R2 b);
#endif
