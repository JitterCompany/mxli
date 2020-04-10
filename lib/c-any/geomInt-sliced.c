//HEADER
/*
  geomInt-sliced.c 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <geomInt.h>
#include <int32Math.h>

//SLICE
P2 p2Project(const P2 d, const P2 v) {
	const int dxv = p2Mul(d,v);
	const int dxd = p2Abs2(d);
	const P2 vd = {{ d.x*dxv/dxd, d.y*dxv/dxd }};
	return vd;
}

//SLICE
P2 p2Quadrant(const P2 p) {
	const P2 q = {{
		p.x > 0 ? 1 : (p.x < 0 ? -1 : 0),
		p.y > 0 ? 1 : (p.y < 0 ? -1 : 0)
	}};
	return q;
}

//SLICE
// direction, position
P2 p2Raster(const P2 d, const P2 p) {
	const P2 q = p2Quadrant(d);
	const int e1 = int32Abs(p2VMul(d, p2AddX(p,q)));
	const int e2 = int32Abs(p2VMul(d, p2AddY(p,q)));
	const int e3 = int32Abs(p2VMul(d, p2Add (p,q)));
	if (e1<e2)
		if (e1<e3) return p2AddX(p,q);
		else return p2Add(p,q);
	else // e2<=e1
		if (e2<e3) return p2AddY(p,q);
		else return p2Add(p,q);
}

//SLICE
R2 r2Normalize(const R2 r) {
	const R2 n = {{
		r.x0<r.x1 ? r.x0 : r.x1,
		r.y0<r.y1 ? r.y0 : r.y1,
		r.x0<r.x1 ? r.x1 : r.x0,
		r.y0<r.y1 ? r.y1 : r.y0
	}};
	return n;
}

//SLICE
R2 r2Intersect(const R2 a, const R2 b) {
	// precondition: contraints mentioned at definition of R2!
	const R2 i = {{
		a.x0>b.x0 ? a.x0 : b.x0,
		a.y0>b.y0 ? a.y0 : b.y0,
		a.x1<b.x1 ? a.x1 : b.x1,
		a.y1<b.y1 ? a.y1 : b.y1
	}};
	return i;
}

//SLICE
R2 r2BoundingBox(const R2 a, const R2 b) {
	// precondition: contraints mentioned at definition of R2!
	const R2 bb = {{
		a.x0<b.x0 ? a.x0 : b.x0,
		a.y0<b.y0 ? a.y0 : b.y0,
		a.x1>b.x1 ? a.x1 : b.x1,
		a.y1>b.y1 ? a.y1 : b.y1
	}};
	return bb;
}
