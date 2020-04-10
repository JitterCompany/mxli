#include <bt.h>
#include <fifoParse.h>
#include <int32Math.h>
#include <bezierUtils.h>
#include <quadratic.h>
#include <int32x2Bezier.h>

enum { EN=20 };

const char* outputFormatSymbols[] = {
	"bt","sbt","psrays","ps","gcode", "b2c", "symbols",
	0
};

Int32x2 btElementBezier2AtSt (BtElement e, Int32 s, Int32 t) {
	if (e.type==BT_TYPE_BEZIER2) {
		const Int32x2 x0 = int32x2 (e.bezier2.x[0], e.bezier2.y[0]);
		const Int32x2 x1 = int32x2 (e.bezier2.x[1], e.bezier2.y[1]);
		const Int32x2 x2 = int32x2 (e.bezier2.x[2], e.bezier2.y[2]);
		return int32x2Add3(
			int32x2Scale_en (EN,x0,int32Mul_en (EN,s,s)),
			int32x2Scale_en (EN,x1,2*int32Mul_en (EN,s,t)),
			int32x2Scale_en (EN,x2,int32Mul_en (EN,t,t))
		);
	}
	else return int32x2Undefined ();
}

BtBezier2Dynamics btElementBezier2Dynamics (BtElement e) {
	Int32x2 v0 = int32x2Undefined();
	Int32x2 v1 = int32x2Undefined();

	switch(e.type) {
		case BT_TYPE_LINE:
		v0 = int32x2 (e.line.x[1]-e.line.x[0],e.line.y[1]-e.line.y[0]);
		v1 = v0;
		break;
		case BT_TYPE_BEZIER2:
		v0 = int32x2MulExp2 (int32x2 (e.bezier2.x[1]-e.bezier2.x[0],e.bezier2.y[1]-e.bezier2.y[0]),1);
		v1 = int32x2MulExp2 (int32x2 (e.bezier2.x[2]-e.bezier2.x[1],e.bezier2.y[2]-e.bezier2.y[1]),1);
		break;
	}
	BtBezier2Dynamics d = {
		.v0 = v0, .v2 = v1,
		.a = int32x2Sub (v1,v0)
	};
	return d;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Manipulation

void btListMap (BtList *list, BtElement (*f)(BtElement e)) {
	const size_t n = btListCanRead (list);
	for (size_t i=0; i<n; i++) {
		BtElement e = btListRead (list);
		btListWrite (list,f(e));
	}
}

BtElement btLineToBezier2 (BtElement e) {
	if (e.type==BT_TYPE_LINE) {
		const Int32 x1 = ((Int64)e.line.x[0]+e.line.x[1]) / 2;
		const Int32 y1 = ((Int64)e.line.y[0]+e.line.y[1]) / 2;
		BtElement eBezier2 = {
			.type = BT_TYPE_BEZIER2,
			.v0 = e.v0,
			.v1 = e.v1,
			.level = e.level,
			{	.bezier2 = {
					.x = { e.line.x[0],	x1,	e.line.x[1]	},
					.y = { e.line.y[0],	y1,	e.line.y[1]	},
				}
			}
		};
		return eBezier2;
	}
	return e;
}

BtElement btElementReverse (BtElement e) {
	BtElement r = e;
	r.v0 = e.v1;
	r.v1 = e.v0;
	switch (e.type) {
		case BT_TYPE_LINE:
					r.line.x[0] = e.line.x[1];
					r.line.x[1] = e.line.x[0];
					r.line.y[0] = e.line.y[1];
					r.line.y[1] = e.line.y[0];
					break;

		case BT_TYPE_BEZIER2:
					r.bezier2.x[0] = e.bezier2.x[2];
					r.bezier2.x[1] = e.bezier2.x[1];
					r.bezier2.x[2] = e.bezier2.x[0];
					r.bezier2.y[0] = e.bezier2.y[2];
					r.bezier2.y[1] = e.bezier2.y[1];
					r.bezier2.y[2] = e.bezier2.y[0];
					break;
		default: ;
	}
	return r;
}

void btListReverse (BtList *list) {
	const size_t n = btListCanRead (list);
	BtElement elements[n];
	for (int i=0; i<n; i++) elements[n-1-i] = btElementReverse (btListRead (list));
	for (int i=0; i<n; i++) btListWrite (list,elements[i]);
}

bool btListExpand (BtList *list, bool (*expand)(BtList *dest, BtElement e)) {
	const size_t n = btListCanRead (list);
	for (size_t i=0; i<n; i++) {
		BtElement e = btListRead (list);
		if (!expand (list,e)) return false;
	}
	return true;
}

bool btExpandDeCasteljau (BtList *dest, BtElement e, Uint32 maxError) {
	if (e.type==BT_TYPE_BEZIER2) {
		Int32 vMid = (Int32)((Int64)e.v0 + e.v1)/2;
		const Int32* xs = e.bezier2.x;
		const Int32* ys = e.bezier2.y;
		const Int32
			ex = xs[0]/4+xs[2]/4-xs[1]/2,			// (x0+x2-2*x1) / 4 ; error at t=1/2
			ey = ys[0]/4+ys[2]/4-ys[1]/2,			// (y0+y2-2*y1) / 4 ; error at t=1/2
			x0 = xs[0],
			x1 = (Int32) ( ((Int64)xs[0]+xs[1])/2 ),
			x3 = (Int32) ( ((Int64)xs[1]+xs[2])/2 ),
			x2 = (Int32) ( ((Int64)x1+x3)/2 ),
			x4 = xs[2],
			y0 = ys[0],
			y1 = (Int32) ( ((Int64)ys[0]+ys[1])/2 ),
			y3 = (Int32) ( ((Int64)ys[1]+ys[2])/2 ),
			y2 = (Int32) ( ((Int64)y1+y3)/2 ),
			y4 = ys[2];

		// maximum coordinate component error
		const Uint32 error = uint32Max (int32Abs(ex), int32Abs(ey) );

		BtElement e0 = {
			.type = BT_TYPE_BEZIER2,
			.v0 = e.v0,
			.v1 = vMid,
			.level = e.level,
			{	.bezier2 = {
					.x = { x0,x1,x2	},
					.y = { y0,y1,y2	},
				}
			}
		};
		BtElement e1 = {
			.type = BT_TYPE_BEZIER2,
			.v0 = vMid,
			.v1 = e.v1,
			.level = e.level,
			{	.bezier2 = {
					.x = { x2,x3,x4	},
					.y = { y2,y3,y4 },
				}
			}
		};

		if (error<=maxError) return btListWriteSafe (dest,e);				// precise enough
		else	// not precise enough -> split in two
			return	btExpandDeCasteljau (dest,e0,maxError)
				&& btExpandDeCasteljau (dest,e1,maxError);
	}
	else return btListWriteSafe (dest,e);
}

bool btExpandDeCasteljauLength (BtList *dest, BtElement e, Uint32 maxLength) {
	if (e.type==BT_TYPE_LINE) {
		const Int32* xs = e.line.x;
		const Int32* ys = e.line.y;
		const Int32x2 v = int32x2 (xs[1]-xs[0],ys[1]-ys[0]);
		if (int32x2Abs (v) > maxLength) {
			const Int32 vMid = (Int32)((Int64)e.v0 + e.v1)/2;
			const Int32
				x0 = xs[0],
				x1 = (Int32) ( ((Int64)xs[0]+xs[1])/2 ),
				x2 = xs[1],
				y0 = ys[0],
				y1 = (Int32) ( ((Int64)ys[0]+ys[1])/2 ),
				y2 = ys[1];

			BtElement e0 = {
				.type = BT_TYPE_LINE,
				.v0 = e.v0,
				.v1 = vMid,
				.level = e.level,
				{	.line = {
						.x = { x0,x1	},
						.y = { y0,y1	},
					}
				}
			};
			BtElement e1 = {
				.type = BT_TYPE_LINE,
				.v0 = vMid,
				.v1 = e.v1,
				.level = e.level,
				{	.line = {
						.x = { x1,x2	},
						.y = { y1,y2	},
					}
				}
			};

			return	btExpandDeCasteljauLength (dest,e0,maxLength)
				&& btExpandDeCasteljauLength (dest,e1,maxLength);
		}
		else return btListWriteSafe (dest,e);	// short enough
	}
	else if (e.type==BT_TYPE_BEZIER2) {
		Int32 vMid = (Int32)((Int64)e.v0 + e.v1)/2;
		const Int32* xs = e.bezier2.x;
		const Int32* ys = e.bezier2.y;
		const Int32x2
			u0 = int32x2 (xs[1]-xs[0],ys[1]-ys[0]),
			u2 = int32x2 (xs[2]-xs[1],ys[2]-ys[1]);
		if (int32x2Abs (u0)>maxLength || int32x2Abs (u2)>maxLength) {
			const Int32
				x0 = xs[0],
				x1 = (Int32) ( ((Int64)xs[0]+xs[1])/2 ),
				x3 = (Int32) ( ((Int64)xs[1]+xs[2])/2 ),
				x2 = (Int32) ( ((Int64)x1+x3)/2 ),
				x4 = xs[2],
				y0 = ys[0],
				y1 = (Int32) ( ((Int64)ys[0]+ys[1])/2 ),
				y3 = (Int32) ( ((Int64)ys[1]+ys[2])/2 ),
				y2 = (Int32) ( ((Int64)y1+y3)/2 ),
				y4 = ys[2];

			BtElement e0 = {
				.type = BT_TYPE_BEZIER2,
				.v0 = e.v0,
				.v1 = vMid,
				.level = e.level,
				{	.bezier2 = {
						.x = { x0,x1,x2	},
						.y = { y0,y1,y2	},
					}
				}
			};
			BtElement e1 = {
				.type = BT_TYPE_BEZIER2,
				.v0 = vMid,
				.v1 = e.v1,
				.level = e.level,
				{	.bezier2 = {
						.x = { x2,x3,x4	},
						.y = { y2,y3,y4 },
					}
				}
			};

			return	btExpandDeCasteljauLength (dest,e0,maxLength)
				&& btExpandDeCasteljauLength (dest,e1,maxLength);
		}
		else return btListWriteSafe (dest,e);	// no splitting
	}
	else return btListWriteSafe (dest,e);
}

bool btExpandDeCasteljauPredicate (BtList *dest, BtElement e, bool (*predicate)(BtElement e)) {
	if (predicate(e)) return btListWriteSafe (dest,e);	// good enough, already
	if (e.type==BT_TYPE_LINE) {
		const Int32* xs = e.line.x;
		const Int32* ys = e.line.y;
		const Int32 vMid = (Int32)((Int64)e.v0 + e.v1)/2;
		const Int32
			x0 = xs[0],
			x1 = (Int32) ( ((Int64)xs[0]+xs[1])/2 ),
			x2 = xs[1],
			y0 = ys[0],
			y1 = (Int32) ( ((Int64)ys[0]+ys[1])/2 ),
			y2 = ys[1];

		BtElement e0 = {
			.type = BT_TYPE_LINE,
			.v0 = e.v0,
			.v1 = vMid,
			.level = e.level,
			{	.line = {
					.x = { x0,x1	},
					.y = { y0,y1	},
				}
			}
		};
		BtElement e1 = {
			.type = BT_TYPE_LINE,
			.v0 = vMid,
			.v1 = e.v1,
			.level = e.level,
			{	.line = {
					.x = { x1,x2	},
					.y = { y1,y2	},
				}
			}
		};

		return	btExpandDeCasteljauPredicate (dest,e0,predicate)
			&& btExpandDeCasteljauPredicate (dest,e1,predicate);
	}
	else if (e.type==BT_TYPE_BEZIER2) {
		Int32 vMid = (Int32)((Int64)e.v0 + e.v1)/2;
		const Int32* xs = e.bezier2.x;
		const Int32* ys = e.bezier2.y;
		const Int32
			x0 = xs[0],
			x1 = (Int32) ( ((Int64)xs[0]+xs[1])/2 ),
			x3 = (Int32) ( ((Int64)xs[1]+xs[2])/2 ),
			x2 = (Int32) ( ((Int64)x1+x3)/2 ),
			x4 = xs[2],
			y0 = ys[0],
			y1 = (Int32) ( ((Int64)ys[0]+ys[1])/2 ),
			y3 = (Int32) ( ((Int64)ys[1]+ys[2])/2 ),
			y2 = (Int32) ( ((Int64)y1+y3)/2 ),
			y4 = ys[2];

		BtElement e0 = {
			.type = BT_TYPE_BEZIER2,
			.v0 = e.v0,
			.v1 = vMid,
			.level = e.level,
			{	.bezier2 = {
					.x = { x0,x1,x2	},
					.y = { y0,y1,y2	},
				}
			}
		};
		BtElement e1 = {
			.type = BT_TYPE_BEZIER2,
			.v0 = vMid,
			.v1 = e.v1,
			.level = e.level,
			{	.bezier2 = {
					.x = { x2,x3,x4	},
					.y = { y2,y3,y4 },
				}
			}
		};

		return	btExpandDeCasteljauPredicate (dest,e0,predicate)
			&& btExpandDeCasteljauPredicate (dest,e1,predicate);
	}
	else return false;	// cannot happen : should be split but is neither line nor bezier2 ????
}


bool btExpandToLines (BtList *dest, BtElement e) {
	if (e.type==BT_TYPE_BEZIER2) {
		Int32 vMid = (Int32)((Int64)e.v0 + e.v1)/2;
		BtElement e0 = {
			.type = BT_TYPE_LINE,
			.v0 = e.v0,
			.v1 = vMid,
			.level = e.level,
			{	.line = {
					.x = { e.bezier2.x[0], e.bezier2.x[1]	},
					.y = { e.bezier2.y[0], e.bezier2.y[1]	},
				}
			}
		};
		BtElement e1 = {
			.type = BT_TYPE_LINE,
			.v0 = vMid,
			.v1 = e.v1,
			.level = e.level,
			{	.line = {
					.x = { e.bezier2.x[1], e.bezier2.x[2]	},
					.y = { e.bezier2.y[1], e.bezier2.y[2]	},
				}
			}
		};
		return btListWriteSafe (dest,e0) && btListWriteSafe (dest,e1);
	}
	else return btListWriteSafe (dest,e);
}


//SLICE
bool btToolRadiusCompensateRight (BtList *dest, Int32 r_e20, Int32x2 *w1,  BtElement e) {
	if (e.type==BT_TYPE_LINE) {
		const Int32x2 x0 = { e.line.x[0], e.line.y[0] };
		const Int32x2 x1 = { e.line.x[1], e.line.y[1] };
		const Int32x2 r0  = int32x2ToolOffset_en (EN,-r_e20,x0,x1);
		const Int32x2 r_1 = int32x2IsDefined (*w1) ? int32x2ToolOffset_en (EN,-r_e20,*w1,x0) : r0;

		const Int32x2 y0 = int32x2Add (x0,r0);
		const Int32x2 y1 = int32x2Add (x1,r0);

		// do we need a bridge-segment?
		// yes, if yn1 and y0 have a measurable distance
		const Uint32 dist_en = int32x2Dist (r_1,r0);
		if (int32x2IsDefined (*w1)	// start at natural tool position, if not defined
		&& dist_en >= 1u<<EN/2) {	// rough estimate... :o)
			const Int32x2 b0 = int32x2Add (x0, r_1);
			const Int32x2 b1 = int32x2Add (x0, int32x2Miter_en (EN, r_1,r0));
			const Int32x2 b2 = int32x2Add (x0, r0);

			if (!btListWriteSafe (dest, btElementBezier2 (e.v0,e.v0, e.level, b0,b1,b2))) return false;
		}

		// normal segment. Miter join
		// store the translated segment
		if (!btListWriteSafe (dest, btElementLine (e.v0,e.v1, e.level, y0,y1))) return false;

		// next iteration
		*w1 = x0;

		return true;
	}
	else if (e.type==BT_TYPE_BEZIER2) {
		const Int32x2 x0 = { e.bezier2.x[0], e.bezier2.y[0] };
		const Int32x2 x1 = { e.bezier2.x[1], e.bezier2.y[1] };
		const Int32x2 x2 = { e.bezier2.x[2], e.bezier2.y[2] };

		const Int32x2 r0  = int32x2ToolOffset_en (EN,-r_e20,x0,x1);
		const Int32x2 r_1 = int32x2IsDefined (*w1) ? int32x2ToolOffset_en (EN,-r_e20,*w1,x0) : r0;
		const Int32x2 r2  = int32x2ToolOffset_en (EN,-r_e20,x1,x2);

		const Int32x2 y0 = int32x2Add (x0,r0);
		const Int32x2 y1 = int32x2Add (x1, int32x2Miter_en (EN, r0,r2));
		const Int32x2 y2 = int32x2Add (x2,r2);

		// do we need a bridge-segment?
		// yes, if yn1 and y0 have a measurable distance
		const Uint32 dist_en = int32x2Dist (r_1,r0);
		if (int32x2IsDefined (*w1)	// start at natural tool position, if not defined
		&& dist_en >= 1u<<EN/2) {	// rough estimate... :o)
			const Int32x2 b0 = int32x2Add (x0, r_1);
			const Int32x2 b1 = int32x2Add (x0, int32x2Miter_en (EN, r_1,r0));
			const Int32x2 b2 = int32x2Add (x0, r0);

			if (!btListWriteSafe (dest, btElementBezier2 (e.v0,e.v0, e.level, b0,b1,b2))) return false;
		}

		// normal segment. Miter join
		// store the translated segment
		if (!btListWriteSafe (dest, btElementBezier2 (e.v0,e.v1, e.level, y0,y1,y2))) return false;

		// next iteration
		*w1 = x1;

		return true;
	}
	else return btListWriteSafe (dest,e);	// unmodified: non-geometric elements
}


bool btElementSplitAt (BtSplitContext *ctx, BtList *dest, Int32x2 p_e20, BtElement e) {
// TODO: use errorLimit!!!!! :o)
	bool success = true;

	// scratchpad fifo
	char buffer[10000];
	Fifo fifo = { buffer, sizeof buffer };

	switch (e.type) {

	case BT_TYPE_LINE: {
		// linear interpolation of the speed
		const Int32x2 x0 = int32x2 (e.line.x[0],e.line.y[0]);
		const Int32x2 x1 = int32x2 (e.line.x[1],e.line.y[1]);
		const Int32 l0 = int32x2Abs (int32x2Sub (x0,p_e20));
		const Int32 l1 = int32x2Abs (int32x2Sub (p_e20,x1));
		const bool splitX0 = l0<=ctx->eps;	// split point is identical to x0
		const bool splitX1 = l1<=ctx->eps;	// split point is identical to x1;

		const Int32 vMid = int64Div ((Int64)e.v1*l0 + (Int64)e.v0*l1, l0+l1);
		BtElement e0 = {
			.type = BT_TYPE_LINE, .v0 = e.v0, .v1 = vMid, .level = e.level,
			{
				.line = {	.x = { x0.x, p_e20.x }, .y = { x0.y, p_e20.y }	},
			}
		};
		BtElement e1 = {
			.type = BT_TYPE_LINE, .v0 = vMid, .v1 = e.v1, .level = e.level,
			{
				.line = {	.x = { p_e20.x, x1.x }, .y = { p_e20.y, x1.y }	},
			}
		};
		ctx->index++;
		if (!splitX0 && !splitX1) success = success && btListWriteSafe (dest,e0);	// normally splittable, part 1
		if (splitX1) success = success && btListWriteSafe (dest,e);				// abnormal split @x1: use original element, labels afterwards
		// insert label and/or comment
		if (ctx->doLabel) {
			fifoPrintString (&fifo,ctx->textLabel?ctx->textLabel:"split_");
			fifoPrintInt32 (&fifo,ctx->index,1);
			const char* string = stringTableWriteFifoUnique (ctx->stringTable,&fifo);
			fifoReset (&fifo);
			if (string==0) return false;
			success = success && btListWriteSafe (dest, btElementLabel (e.level, string));
		}
		if (ctx->doComment) {
			fifoPrintString (&fifo,ctx->textComment?ctx->textComment:"split_");
			fifoPrintInt32 (&fifo,ctx->index,1);
			const char* string = stringTableWriteFifoUnique (ctx->stringTable,&fifo);
			fifoReset (&fifo);
			if (string==0) return false;
			success = success && btListWriteSafe (dest, btElementComment (e.level, string));
		}
		if (splitX0) success = success && btListWriteSafe (dest,e);	// abnormal split @x0: use original element, labels already written
		if (!splitX0 && !splitX1) success = success && btListWriteSafe (dest,e1);	// normally splittable, part 2
		return success;
	} break;

	case BT_TYPE_BEZIER2: {
		// linear interpolation of the speed
		const Int32x2 x0 = int32x2 (e.bezier2.x[0],e.bezier2.y[0]);
		const Int32x2 x1 = int32x2 (e.bezier2.x[1],e.bezier2.y[1]);
		const Int32x2 x2 = int32x2 (e.bezier2.x[2],e.bezier2.y[2]);
		const Int32x2 a = int32x2Sub (int32x2Add (x0,x2), int32x2Scale (x1,2));
		Int32x2 q;	// projection of p onto straight line x0->x2
		if (int32x2IntersectLineLine_en (EN, x0, int32x2Sub (x1,x0), p_e20, a, &q)) {	// this has to exist!
			const Int32 l0 = int32x2Abs (int32x2Sub (x0,q));
			const Int32 l1 = int32x2Abs (int32x2Sub (q,x2));
			const bool splitX0 = l0<=ctx->eps;	// split point is identical to x0
			const bool splitX1 = l1<=ctx->eps;	// split point is identical to x1;

			const Int32 vMid = int64Div ((Int64)e.v1*l0 + (Int64)e.v0*l1, l0+l1);
			const Int32 x01x = int64Div ((Int64)l0*x1.x + (Int64)l1*x0.x, l0+l1); 
			const Int32 x01y = int64Div ((Int64)l0*x1.y + (Int64)l1*x0.y, l0+l1); 
			const Int32 x12x = int64Div ((Int64)l0*x2.x + (Int64)l1*x1.x, l0+l1); 
			const Int32 x12y = int64Div ((Int64)l0*x2.y + (Int64)l1*x1.y, l0+l1); 
			BtElement e0 = {
				.type = BT_TYPE_BEZIER2, .v0 = e.v0, .v1 = vMid, .level = e.level,
				{
					.bezier2 = {	.x = { x0.x, x01x, p_e20.x }, .y = { x0.y, x01y, p_e20.y }	},
				}
			};
			BtElement e1 = {
				.type = BT_TYPE_BEZIER2, .v0 = vMid, .v1 = e.v1, .level = e.level,
				{
					.bezier2 = {	.x = { p_e20.x, x12x, x2.x }, .y = { p_e20.y, x12y, x2.y }	},
				}
			};
			ctx->index++;

			if (!splitX0 && !splitX1) success = success && btListWriteSafe (dest,e0);	// normally splittable, part 1
			if (splitX1) success = success && btListWriteSafe (dest,e);				// abnormal split @x1: use original element, labels afterwards
			// insert label and/or comment
			if (ctx->doLabel) {
				fifoPrintString (&fifo,ctx->textLabel?ctx->textLabel:"split_");
				fifoPrintInt32 (&fifo,ctx->index,1);
				const char* string = stringTableWriteFifoUnique (ctx->stringTable,&fifo);
				fifoReset (&fifo);
				if (string==0) return false;
				success = success && btListWriteSafe (dest, btElementLabel (e.level, string));
			}
			if (ctx->doComment) {
				fifoPrintString (&fifo,ctx->textComment?ctx->textComment:"split_");
				fifoPrintInt32 (&fifo,ctx->index,1);
				const char* string = stringTableWriteFifoUnique (ctx->stringTable,&fifo);
				fifoReset (&fifo);
				if (string==0) return false;
				success = success && btListWriteSafe (dest, btElementComment (e.level, string));
			}
			if (splitX0) success = success && btListWriteSafe (dest,e);	// abnormal split @x0: use original element, labels already written
			if (!splitX0 && !splitX1) success = success && btListWriteSafe (dest,e1);	// normally splittable, part 2
			return success;
		}
		else return false;	// impossible for point on curve...
	} break;

	default:	return btListWriteSafe (dest,e);
	}
}

/** Calculates the intersection point of a line/curve and a 'beam' from point origin.
 * Only the first contact point is calculates and there can be at most one such point.
 * @param e the curve element exposed to the beam
 * @param origin_e20 the source of the beam
 * @param beam_e20 the beam direction. Length doesn't matter here.
 * @return the point where the beam hits the curve, or int32Undefined() if no such point exists.
 */
Int32x2 btElementIntersectBeam (BtElement e, Int32x2 origin_e20, Int32x2 beam_e20) {
	switch (e.type) {
		case BT_TYPE_LINE: {
			const Int32x2 x0 = int32x2 (e.line.x[0],e.line.y[0]);
			const Int32x2 x1 = int32x2 (e.line.x[1],e.line.y[1]);
			Int32x2 p;
			if (int32x2IntersectLineSegment_en (EN,origin_e20,beam_e20,x0,x1,&p)) {
				return p;
			}
			else return int32x2Undefined();
		} break;

		case BT_TYPE_BEZIER2: {
			const Int32x2 x0 = int32x2 (e.bezier2.x[0],e.bezier2.y[0]);
			const Int32x2 x1 = int32x2 (e.bezier2.x[1],e.bezier2.y[1]);
			const Int32x2 x2 = int32x2 (e.bezier2.x[2],e.bezier2.y[2]);
			// first check neccessary condition easily
			if (int32x2IntersectLineSegment_en (EN,origin_e20,beam_e20,x0,x1,0)
			|| int32x2IntersectLineSegment_en (EN,origin_e20,beam_e20,x1,x2,0)
			|| int32x2IntersectLineSegment_en (EN,origin_e20,beam_e20,x0,x2,0)) {
				// OK now the hard way, quadratic equation
				// a*t^2 + b*t + c = 0 , solve for t1/2
				// a = (x0-2*x1+x2) x beam
				// b = (2*x1-2*x0) x beam
				// c = (x0-origin) x beam
				const Int32 a = int32x2MulVector_en (EN,beam_e20,
					int32x2Sub ( int32x2Add(x0,x2), int32x2MulExp2(x1,1) )
				);
				const Int32 b = int32x2MulVector_en (EN,beam_e20,
					int32x2MulExp2 ( int32x2Sub(x1,x0), 1)
				);
				const Int32 c = int32x2MulVector_en (EN,beam_e20,
					int32x2Sub (x0,origin_e20)
				);
				const Int32x2 t12 = int32MidnightAbc_en (EN,a,b,c);
				// unproven: which one hits first can be determined by looking at x0 and x2, only!
				Int32 t;
				if (0<=t12.x && t12.x<=(1<<EN)) {	// t1 is candidate
					t = t12.x;
					if (0<=t12.y && t12.y<=(1<<EN)) {	// t2 is candidate, too! Take the closer one
						// scalar product is cheaper than |x|, so we use it as a substitute
						const Int32 d0 = int32x2MulScalar_en (EN,beam_e20, int32x2Sub (x0,origin_e20));
						const Int32 d2 = int32x2MulScalar_en (EN,beam_e20, int32x2Sub (x2,origin_e20));
						if (d2<d0) t = t12.y;	// second one is better (closer)!
					}
					// calculate point from t
					return int32x2Bezier2ValueAt_een(EN,EN,x0,x1,x2,t);
				}
				else if (0<=t12.y && t12.y<=(1<<EN)) {	// t2 is the only candidate
					// calculate point from t
					return int32x2Bezier2ValueAt_een(EN,EN,x0,x1,x2,t12.y);
				}
				else return int32x2Undefined();	// no intersection; :o) This cannot happen, because we checked, above!!!

			}
			else return int32x2Undefined();	// no intersection
		} break;

		default: return int32x2Undefined();
	}
}

/** Finds the point on a path element that has a tangent of the desired angle and direction (+-).
 * @param tangent the Bezier path element local speed orientation and direction (forward, only)
 * @param pathElement a path element
 * @return the tangent point or int32x2Undefined if no such point (uniquely) exists.
 */
Int32x2 btElementTouchTangent (Int32 eps, Int32x2 tangent, BtElement pathElement) {
	if (pathElement.type==BT_TYPE_BEZIER2) {
		const BtBezier2Dynamics dyn = btElementBezier2Dynamics (pathElement);
		const Int32 n_en = int32x2MulVector_en (EN,tangent,dyn.a);
		if (int32x2Abs (dyn.a)>eps		// nonzero acceleration
		&& int32Abs (n_en) > eps) {		// required for division below ; if not satisfied, then t,s out of range so stay cool ;-)
			const Int32 t_en = int32Div_en (EN,int32x2MulVector_en (EN,dyn.v0,tangent),n_en);
			const Int32 s_en = int32Div_en (EN,int32x2MulVector_en (EN,tangent,dyn.v2),n_en);
			const Int32x2 vt = int32x2Add (
				int32x2Scale_en (EN,dyn.v0,s_en),
				int32x2Scale_en (EN,dyn.v2,t_en)
			);
			if (t_en>=0 && s_en>=0				// tangent within segment
			&& int32x2MulScalar_e2n (vt,tangent)>=0 ) {	// speed with correct sign
				return btElementBezier2AtSt (pathElement,s_en,t_en);
			}
			else return int32x2Undefined();
		}
		else {	// no acceleration or out of range: line or really straight bezier
			return int32x2Undefined();
		}
	}
	else return int32x2Undefined();
}

Int32x2 btElementTouchDistance (Int32 eps, Int32x2 normal, BtElement pathElement) {
	if (!btElementIsPath(pathElement)) return int32x2Undefined();
	else {
		Int32x2 p = btElementPointBegin (pathElement);

		// possibly better alternatives...
		Int32x2 alt;
		alt = btElementPointEnd (pathElement);
		if (int32x2MulScalar_e2n (normal, int32x2Sub (alt,p)) < 0 ) p = alt;	// alt is closer

		alt = btElementTouchTangent (eps, int32x2j (normal), pathElement);
		if (int32x2IsDefined (alt) && int32x2MulScalar_e2n (normal, int32x2Sub (alt,p)) < 0 ) p = alt;	// alt is closer

		alt = btElementTouchTangent (eps, int32x2jjj (normal), pathElement);
		if (int32x2IsDefined (alt) && int32x2MulScalar_e2n (normal, int32x2Sub (alt,p)) < 0 ) p = alt;	// alt is closer

		return p;
	}
}

bool btListSplitAtHitPoints (BtSplitContext *splitContext, BtList *list, BtHitPointFifo *hitPoints) {
	bool success = true;
	// do the splitting
	// :o) only one split per segment allowed
	int segIndex = 0;
	for (int n=btListCanRead (list); n>0; n--, segIndex++) {
		const BtElement e = btListRead (list);
		if (btHitPointFifoCanFindHitPoint (hitPoints,segIndex)) {
			const Int32x2 splitPoint = btHitPointFifoFindHitPoint (hitPoints,segIndex);	// find and remove
			success = success && btElementSplitAt (splitContext,list,splitPoint,e);
		}
		else success = success && btListWriteSafe (list,e);	// unmodified
	}
	return success;
}

BtCurveTangent btElementCurveTangentBegin (BtElement e) {
	BtCurveTangent t = {
		.v = e.v0,
		.level = e.level,
	};
	switch (e.type) {
		case BT_TYPE_LINE:
			t.x = int32x2 (e.line.x[0], e.line.y[0]);
			t.t = int32x2 (e.line.x[1]-e.line.x[0], e.line.y[1]-e.line.y[0]);
		break;

		case BT_TYPE_BEZIER2:
			t.x = int32x2 (e.bezier2.x[0], e.bezier2.y[0]);
			t.t = int32x2 (2*(e.bezier2.x[1]-e.bezier2.x[0]), 2*(e.bezier2.y[1]-e.bezier2.y[0]));
		break;
	}
	return t;
}

BtCurveTangent btElementCurveTangentEnd (BtElement e) {
	BtCurveTangent t = {
		.v = e.v1,
		.level = e.level,
	};
	switch (e.type) {
		case BT_TYPE_LINE:
			t.x = int32x2 (e.line.x[1], e.line.y[1]);
			t.t = int32x2 (e.line.x[1]-e.line.x[0], e.line.y[1]-e.line.y[0]);
		break;

		case BT_TYPE_BEZIER2:
			t.x = int32x2 (e.bezier2.x[2], e.bezier2.y[2]);
			t.t = int32x2 (2*(e.bezier2.x[2]-e.bezier2.x[1]), 2*(e.bezier2.y[2]-e.bezier2.y[1]));
		break;
	}
	return t;
}

int btListIndexFirstPath (BtList *list) {
	BtList clone = *list;
	for (int i=0; blockFifoCanRead(&clone); i++) {
		BtElement e = btListRead (&clone);
		if (btElementIsPath (e)) return i; 
	}
	return -1;	// no path element found
}

int btListIndexLastPath (BtList *list) {
	BtList clone = *list;
	int lastPath = -1;
	for (int i=0; blockFifoCanRead(&clone); i++) {
		BtElement e = btListRead (&clone);
		if (btElementIsPath (e)) lastPath = i; 
	}
	return lastPath;
}

bool btListPrependPathPointWithExcentricity (BtList *list, Int32x2 p, Int32 excentricity_e20) {
	// first pass: find first and last path element by index
	const int firstPath = btListIndexFirstPath (list);
	if (firstPath==-1) return false;
	const int n = btListCanRead (list);
	for (int i=0; i<n; i++) {
		BtElement e = btListRead (list);
		if (i==firstPath) {
			BtCurveTangent t = btElementCurveTangentBegin (e);
			if (excentricity_e20==0) {
				BtElement ePrev = {
					.type = BT_TYPE_LINE,
					.v0 = e.v0,
					.v1 = e.v0,
					.level = e.level,
					{	.line = {
							.x = { p.x, t.x.x },
							.y = { p.y, t.x.y },
						}
					}
				};
				if (!btListWriteSafe (list,ePrev) || !btListWriteSafe (list,e)) return false;
			}
			else {
				const Int32x2 x1 = int32x2Sub (t.x, int32x2ScaleTo_en (EN, t.t, excentricity_e20));
				BtElement ePrev = {
					.type = BT_TYPE_BEZIER2,
					.v0 = e.v0,
					.v1 = e.v0,
					.level = e.level,
					{	.bezier2 = {
							.x = { p.x, x1.x, t.x.x },
							.y = { p.y, x1.y, t.x.y },
						}
					}
				};
				if (!btListWriteSafe (list,ePrev) || !btListWriteSafe (list,e)) return false;
			}
		}
		else {	// put back element
			if (!btListWriteSafe (list,e)) return false;
		}
	}
	return true;
}

bool btListAppendPathPointWithExcentricity (BtList *list, Int32x2 p, Int32 excentricity_e20) {
	// first pass: find first and last path element by index
	const int lastPath = btListIndexLastPath (list);
	if (lastPath==-1) return false;
	const int n = btListCanRead (list);
	for (int i=0; i<n; i++) {
		BtElement e = btListRead (list);
		if (!btListWriteSafe (list,e)) return false;	// put back original element
		// and parhaps add an additional one
		if (i==lastPath) {
			BtCurveTangent t = btElementCurveTangentEnd (e);
			if (excentricity_e20==0) {
				BtElement eNext = {
					.type = BT_TYPE_LINE,
					.v0 = e.v1,
					.v1 = e.v1,
					.level = e.level,
					{	.line = {
							.x = { t.x.x, p.x },
							.y = { t.x.y, p.y },
						}
					}
				};
				if (!btListWriteSafe (list,eNext)) return false;
			}
			else {
				const Int32x2 x1 = int32x2Add (t.x, int32x2ScaleTo_en (EN, t.t, excentricity_e20));
				BtElement eNext = {
					.type = BT_TYPE_BEZIER2,
					.v0 = e.v1,
					.v1 = e.v1,
					.level = e.level,
					{	.bezier2 = {
							.x = { t.x.x, x1.x, p.x },
							.y = { t.x.y, x1.y, p.y },
						}
					}
				};
				if (!btListWriteSafe (list,eNext)) return false;
			}
		}
	}
	return true;
}

bool btListAppendPathPointWithDirection (BtList *list, Int32x2 p, Int32x2 direction);
bool btListPrependPathPointWithDirection (BtList *list, Int32x2 p, Int32x2 direction);

BtElement btElementFuse (bool homogenous, Int32 errorLimit, BtElement a, BtElement b) {
	if (a.level==b.level
	&& a.v1==b.v0) {
		if (a.type==BT_TYPE_LINE && b.type==BT_TYPE_LINE && a.level==b.level) {
			const Int32x2 va = int32x2( a.line.x[1]-a.line.x[0], a.line.y[1]-a.line.y[0]);
			const Int32x2 vb = int32x2( b.line.x[1]-b.line.x[0], b.line.y[1]-b.line.y[0]);
			const Int32 area = int32x2MulVector_en (EN,va,vb);
			const Int32 l = int32x2Abs (int32x2Add (va,vb));
			if (int32Abs (int32Div_en(EN,area,l)) < errorLimit) return btElementLine (
				a.v0, b.v1, a.level,
				int32x2 (a.line.x[0],a.line.y[0]), int32x2 (b.line.x[1],b.line.y[1])
			);
			else return btElementUndefined();	// geometric mismatch
		}
		else if (a.type==BT_TYPE_BEZIER2 && b.type==BT_TYPE_BEZIER2) {
			const Int32x2 x0 = int32x2( a.bezier2.x[0], a.bezier2.y[0]);
			const Int32x2 x4 = int32x2( b.bezier2.x[2], b.bezier2.y[2]);
			const Int32x2 va0 = int32x2( a.bezier2.x[1]-a.bezier2.x[0], a.bezier2.y[1]-a.bezier2.y[0]);
			const Int32x2 va1 = int32x2( a.bezier2.x[2]-a.bezier2.x[1], a.bezier2.y[2]-a.bezier2.y[1]);
			const Int32x2 vb0 = int32x2( b.bezier2.x[1]-b.bezier2.x[0], b.bezier2.y[1]-b.bezier2.y[0]);
			const Int32x2 vb1 = int32x2( b.bezier2.x[2]-b.bezier2.x[1], b.bezier2.y[2]-b.bezier2.y[1]);
			const Int32x2 aa = int32x2Sub (va1,va0);
			const Int32x2 ab = int32x2Sub (va1,va0);
			const Int32 area = int32x2MulVector_en (EN,va1,vb0);
			const Int32 l = int32x2Abs (int32x2Add (va1,vb0));
			if (int32Abs (int32Div_en(EN,area,l)) < errorLimit	// parallel tangents
			&& int32x2MulScalar_en (EN,aa,ab) >= 0			// same acceleration direction
			&& int32x2MulVector_en (EN,aa,ab) < errorLimit		// paralles accelerations
			&& (int32Abs (int32x2MulVector_en (EN,aa,vb0) - int32x2MulVector_en (EN,ab,va1)) < errorLimit)	// same parabola shape
			) {
				Int32x2 newX1;
				if (int32x2IntersectLineLine_en (EN,x0,va0,x4,vb1,&newX1)) return btElementBezier2 (a.v0, b.v1, a.level, x0,newX1,x4);
				else return btElementUndefined();
			}
			else return btElementUndefined();	// geometric mismatch
			
		}
		else return btElementUndefined ();
	}
	else return btElementUndefined ();	// limiting factors other than geometric ones
}

Int32 int32SnapToPhase (Int32 angleScale, Int32 approxPhase, Int32 phase) {
	while (phase < approxPhase-angleScale/2) phase += angleScale;
	while (phase > approxPhase+angleScale/2) phase -= angleScale;
	return phase;
}

Int32x2 btElementPolarAngles (BtPolarContext *ctx, BtElement e) {
	switch (e.type) {

	case BT_TYPE_LINE: {
		const Int32x2 x0 = int32x2Sub (int32x2(e.line.x[0],e.line.y[0]),ctx->center);
		const Int32x2 x1 = int32x2Sub (int32x2(e.line.x[1],e.line.y[1]),ctx->center);
		const Int32x2 displacement = int32x2Sub (x1,x0);
		const Int32 phi0 = int32SnapToPhase (ctx->unitAngle,ctx->accumulatedAngle, (Int64)ctx->unitAngle * int32x2Angle_e32 (x0) >> 32);
		Int32 phi1 = int32SnapToPhase (ctx->unitAngle,phi0, (Int64)ctx->unitAngle * int32x2Angle_e32 (x1) >> 32);	// not final value, but at most 1/2 rev off

		// make sure, the rotation direction is correct...
		if (int32x2MulVector (x0,displacement) >= 0) {	// rotation left, mathematically positive
			while (phi1<phi0) phi1 += ctx->unitAngle;	// one revolution off
		}
		else {	// rotation right, mathematically negative
			while (phi1>phi0) phi1 -= ctx->unitAngle;	// one revolution off;
		}
		return int32x2 (phi0,phi1);
	} break;

	case BT_TYPE_BEZIER2: {
		const Int32x2 x0 = int32x2Sub (int32x2(e.bezier2.x[0],e.bezier2.y[0]),ctx->center);
		const Int32x2 x1 = int32x2Sub (int32x2(e.bezier2.x[1],e.bezier2.y[1]),ctx->center);
		const Int32x2 x2 = int32x2Sub (int32x2(e.bezier2.x[2],e.bezier2.y[2]),ctx->center);
		const Int32x2 ray = int32x2Add3 (int32x2MulExp2 (x0,-2), int32x2MulExp2 (x2,-2), int32x2MulExp2 (x1,-1));
		const Int32x2 displacement = int32x2Sub (x2,x0);
		const Int32 phi0 = int32SnapToPhase (ctx->unitAngle,ctx->accumulatedAngle,(Int64)ctx->unitAngle * int32x2Angle_e32 (x0) >> 32);
		Int32 phi2 = int32SnapToPhase (ctx->unitAngle,phi0, (Int64)ctx->unitAngle * int32x2Angle_e32 (x2) >> 32);	// not final value, but at most 1/2 rev off

		// make sure, the rotation direction is correct...
		if (int32x2MulVector (ray,displacement) >= 0) {	// rotation left, mathematically positive
			while (phi2<phi0) phi2 += ctx->unitAngle;	// one revolution off
		}
		else {	// rotation right, mathematically negative
			while (phi2>phi0) phi2 -= ctx->unitAngle;	// one revolution off;
		}
		return int32x2 (phi0,phi2);

	} break;

	default: return int32x2Undefined();
	}
}


BtElement btElementCartesianToPolar (BtPolarContext *context, BtElement e) {
	switch (e.type) {

	case BT_TYPE_LINE: {
		const Int32x2 x0 = int32x2Sub (int32x2(e.line.x[0],e.line.y[0]),context->center);
		if (!int32IsDefined (context->accumulatedAngle)) context->accumulatedAngle = (Int64)context->unitAngle * int32x2Angle_e32 (x0) >> 32;
		const Int32x2 x2 = int32x2Sub (int32x2(e.line.x[1],e.line.y[1]),context->center);

		Int32x2 angles = btElementPolarAngles (context, e);
		context->accumulatedAngle = angles.y;

		const Int32   r0 = int32x2Abs (x0);
		const Int32   r2 = int32x2Abs (x2);
		const Int32x2 e0 = int32x2ScaleTo1_en (EN,x0);	// radius unit vector
		const Int32x2 e2 = int32x2ScaleTo1_en (EN,x2);	// radius unit vector

		const Int32x2 v0 = int32x2Sub (x2,x0);		// Bezier cartesian speed
		const Int32x2 v2 = int32x2Sub (x2,x0);		// Bezier cartesian speed

		const Int32 pix2_en = PIx2_E29>>29-EN;

		// speed through radius change
		const Int32 vr0Value = int32x2MulScalar_en (EN,e0,v0);
		// speed through angular change
		const Int32 vs0Value = int32Div_en (EN, int32x2MulVector_en (EN,e0,v0), int32Mul_en (EN, pix2_en, r0));
		const Int32x2 u0 = { vr0Value, vs0Value };

		// speed through radius change
		const Int32 vr2Value = int32x2MulScalar_en (EN,e2,v2);
		// speed through angular change
		const Int32 vs2Value = int32Div_en (EN, int32x2MulVector_en (EN,e2,v2), int32Mul_en (EN, pix2_en, r2));
		const Int32x2 u2 = { vr2Value, vs2Value };

		const Int32x2 p0 = { r0, angles.x };
		const Int32x2 p2 = { r2, angles.y };

		//const Int32 sineLimit_en = (1<<e) / 16;		// :o) needs to be well chosen!!
		//const Int32 sineLimit_en = (1<<e) / 256;		// :o) needs to be well chosen!! (this is a moderately good one)
		//const Int32 sineLimit_en = (1<<e) / 512;		// :o) needs to be well chosen!! (this is a good one)
		//const Int32 sineLimit_en = (1<<e) / 1024;		// :o) needs to be well chosen!! (this is a good one)
		const Int32 sineLimit_en = (1<<EN) / 2048;		// :o) needs to be well chosen!!
		//const Int32 sineLimit_en = 0;

		const Int32x2 p1 = int32x2IntersectLineLineBetween_en (EN, sineLimit_en, p0,u0, p2,u2);
		// finally we have p1!!!!!!

		return btElementBezier2 (
			int32Mul_en (EN, int32x2Abs(u0), int32Div_en (EN, e.v0,int32x2Abs (v0))),
			int32Mul_en (EN, int32x2Abs(u2), int32Div_en (EN, e.v1,int32x2Abs (v2))),
			e.level, p0,p1,p2
		);
	} break;

	case BT_TYPE_BEZIER2: {
		const Int32x2 x0 = int32x2Sub (int32x2(e.bezier2.x[0],e.bezier2.y[0]),context->center);
		if (!int32IsDefined (context->accumulatedAngle)) context->accumulatedAngle = (Int64)context->unitAngle * int32x2Angle_e32 (x0) >> 32;
		const Int32x2 x1 = int32x2Sub (int32x2(e.bezier2.x[1],e.bezier2.y[1]),context->center);
		const Int32x2 x2 = int32x2Sub (int32x2(e.bezier2.x[2],e.bezier2.y[2]),context->center);
		const Int32   r0 = int32x2Abs (x0);
		const Int32   r2 = int32x2Abs (x2);
		
		// :o) missing zero native speeds, leading to divide by zero...
		const Int32x2 v0= int32x2MulExp2 (int32x2Sub (x1,x0),1);
		const Int32x2 v2= int32x2MulExp2 (int32x2Sub (x2,x1),1);

		const Int32x2 e0 = int32x2ScaleTo1_en (EN,x0);	// radius unit vector
		const Int32x2 e2 = int32x2ScaleTo1_en (EN,x2);	// radius unit vector

		Int32x2 angles = btElementPolarAngles (context, e);
		context->accumulatedAngle = angles.y;

		const Int32 pix2_en = PIx2_E29>>29-EN;

		// speed through radius change
		const Int32 vr0Value = int32x2MulScalar_en (EN,e0,v0);
		// speed through angular change
		const Int32 vs0Value = int32Div_en (EN, int32x2MulVector_en (EN,e0,v0), int32Mul_en (EN, pix2_en, r0));
		const Int32x2 u0= { vr0Value, vs0Value };

		// speed through radius change
		const Int32 vr2Value = int32x2MulScalar_en (EN,e2,v2);
		// speed through angular change
		const Int32 vs2Value = int32Div_en (EN, int32x2MulVector_en (EN,e2,v2), int32Mul_en (EN, pix2_en, r2));
		const Int32x2 u2= { vr2Value, vs2Value };

		const Int32x2 p0 = { r0, angles.x };
		const Int32x2 p2 = { r2, angles.y };

		//const Int32 sineLimit_en = (1<<e) / 16;		// :o) needs to be well chosen!!
		//const Int32 sineLimit_en = (1<<e) / 256;		// :o) needs to be well chosen!! (this is a moderately good one)
		//const Int32 sineLimit_en = (1<<e) / 512;		// :o) needs to be well chosen!! (this is a good one)
		//const Int32 sineLimit_en = (1<<e) / 1024;		// :o) needs to be well chosen!! (this is a good one)
		const Int32 sineLimit_en = (1<<EN) / 2048;		// :o) needs to be well chosen!!
		//const Int32 sineLimit_en = 0;

		const Int32x2 p1 = int32x2IntersectLineLineBetween_en (EN, sineLimit_en, p0,u0, p2,u2);
		// finally we have p1!!!!!!

		return btElementBezier2 (
			int32Mul_en (EN, int32x2Abs(u0), int32Div_en (EN, e.v0,int32x2Abs (v0))),
			int32Mul_en (EN, int32x2Abs(u2), int32Div_en (EN, e.v1,int32x2Abs (v2))),
			e.level, p0,p1,p2
		);
	} break;

	default:	return e;

	}
}

BtElement btElementPolarToCartesian (BtPolarContext *context, BtElement e) {
	switch (e.type) {

	case BT_TYPE_LINE: {
		//return btElementUndefined();
		return btElementComment (0,"Shit happened!!");
	} break;

	// TODO :!!!!!! Skalierung mit unitAngle muss wie 2*PI beruecksichtigt werden bei allen Geschwindigkeiten/Richtungen!!!

	case BT_TYPE_BEZIER2: {
		const Int32x2 p0 = int32x2(e.bezier2.x[0],e.bezier2.y[0]);
		const Int32x2 p1 = int32x2(e.bezier2.x[1],e.bezier2.y[1]);
		const Int32x2 p2 = int32x2(e.bezier2.x[2],e.bezier2.y[2]);
		const Int32 phi0_e32 = int32Div_en (EN,p0.y,context->unitAngle) << 32-EN;
		const Int32 phi2_e32 = int32Div_en (EN,p2.y,context->unitAngle) << 32-EN;
		const Int32x2 e0 = int32x2Rot_en (EN,phi0_e32);	// radius unit vector
		const Int32x2 e2 = int32x2Rot_en (EN,phi2_e32);	// radius unit vector
		const Int32x2 x0 = int32x2Add (context->center, int32x2ScaleTo_en (EN,e0,p0.x));
		const Int32x2 x2 = int32x2Add (context->center, int32x2ScaleTo_en (EN,e2,p2.x));

		//const Int32 pix2_en = int32Div_en (EN, PIx2_E29>>29-EN,context->unitAngle);
		const Int32 pix2_en = int32Div_en (EN, PIx2_E29>>29-EN,context->unitAngle);

		// tangential speeds:
		const Int32x2 u0Native = int32x2MulExp2 ( int32x2Sub (p1,p0), 1);	// 2*(r',s')
		const Int32x2 u2Native = int32x2MulExp2 ( int32x2Sub (p2,p1), 1);

		// speed through radius change
		const Int32x2 vr0 = int32x2Scale_en (EN,e0,u0Native.x);		// e-radial * r'
		// speed through angular change
		const Int32x2 vs0 = int32x2Scale_en (EN, int32x2j(e0), int32Mul3_en (EN, pix2_en, u0Native.y, p0.x));	// e-tangent * 2*PI*s'*r
		const Int32x2 v0Dir = int32x2Add (vr0,vs0);	// not to scale (not needed)

		// speed through radius change
		const Int32x2 vr2 = int32x2Scale_en (EN,e2,u2Native.x);
		// speed through angular change
		const Int32x2 vs2 = int32x2Scale_en (EN, int32x2j(e2), int32Mul3_en (EN, pix2_en, u2Native.y, p2.x));
		const Int32x2 v2Dir = int32x2Add (vr2,vs2);	// not to scale (not needed)

		//const Int32 sineLimit_en = (1<<e) / 16;		// :o) needs to be well chosen!!
		//const Int32 sineLimit_en = (1<<e) / 256;		// :o) needs to be well chosen!! (this is a moderately good one)
		//const Int32 sineLimit_en = (1<<e) / 512;		// :o) needs to be well chosen!! (this is a good one)
		//const Int32 sineLimit_en = (1<<e) / 1024;		// :o) needs to be well chosen!! (this is a good one)
		const Int32 sineLimit_en = (1<<EN) / 2048;		// :o) needs to be well chosen!!
		//const Int32 sineLimit_en = 0;

		const Int32x2 x1 = int32x2IntersectLineLineBetween_en (EN, sineLimit_en, x0,v0Dir, x2,v2Dir);
		// finally we have x1!!!!!!

		const Int32x2 v0 = int32x2MulExp2 (int32x2Sub (x1,x0), 1);
		const Int32x2 v2 = int32x2MulExp2 (int32x2Sub (x2,x1), 1);
		const Int32 scale0 = int32Div_en (EN, e.v0, int32x2Abs (u0Native));	// inverse scaling as used for cartesian -> polar
		const Int32 scale2 = int32Div_en (EN, e.v1, int32x2Abs (u2Native));	// inverse scaling as used for cartesian -> polar
		return btElementBezier2 (
			int32Mul_en (EN,scale0,int32x2Abs (v0)),
			int32Mul_en(EN,scale2,int32x2Abs (v2)),
			e.level, x0,x1,x2
		);
	} break;

	default:	return e;

	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Elliptic coordinates

BtElement btElementCartesianToElliptic (BtEllipticContext *context, BtElement e) {
	switch (e.type) {

	case BT_TYPE_LINE: {
		const Int32x2 x0 = int32x2(e.line.x[0],e.line.y[0]);
		const Int32x2 ax0 = int32x2Sub (x0,context->centerA);
		const Int32x2 bx0 = int32x2Sub (x0,context->centerB);

		const Int32x2 x2 = int32x2(e.line.x[1],e.line.y[1]);
		const Int32x2 ax2 = int32x2Sub (x2,context->centerA);
		const Int32x2 bx2 = int32x2Sub (x2,context->centerB);

		const Int32   ar0 = int32x2Abs (ax0);
		const Int32   ar2 = int32x2Abs (ax2);
		const Int32x2 ae0 = int32x2ScaleTo1_en (EN,ax0);	// radius unit vector
		const Int32x2 ae2 = int32x2ScaleTo1_en (EN,ax2);	// radius unit vector

		const Int32   br0 = int32x2Abs (bx0);
		const Int32   br2 = int32x2Abs (bx2);
		const Int32x2 be0 = int32x2ScaleTo1_en (EN,bx0);	// radius unit vector
		const Int32x2 be2 = int32x2ScaleTo1_en (EN,bx2);	// radius unit vector

		const Int32x2 v0 = int32x2Sub (x2,x0);		// Bezier cartesian speed
		const Int32x2 v2 = int32x2Sub (x2,x0);		// Bezier cartesian speed

		const Int32 av0Value = int32x2MulScalar_en (EN,ae0,v0);
		const Int32 bv0Value = int32x2MulScalar_en (EN,be0,v0);
		const Int32 av2Value = int32x2MulScalar_en (EN,ae2,v2);
		const Int32 bv2Value = int32x2MulScalar_en (EN,be2,v2);

		const Int32 sineLimit_en = (1<<EN) / 2048;		// :o) needs to be well chosen!!

		const Int32x2 ell0 = int32x2 (ar0,br0);
		const Int32x2 ell2 = int32x2 (ar2,br2);
		const Int32x2 ellv0 = int32x2 (av0Value,bv0Value);
		const Int32x2 ellv2 = int32x2 (av2Value,bv2Value);

		const Int32x2 ell1 = int32x2IntersectLineLineBetween_en (EN, sineLimit_en, ell0,ellv0, ell2,ellv2);

		return btElementBezier2 (
			int32Mul_en (EN, int32x2Abs(ellv0), int32Div_en (EN, e.v0,int32x2Abs (v0))),
			int32Mul_en (EN, int32x2Abs(ellv2), int32Div_en (EN, e.v1,int32x2Abs (v2))),
			e.level, ell0,ell1,ell2
		);
	} break;

	case BT_TYPE_BEZIER2: {
		const Int32x2 x0 = int32x2(e.bezier2.x[0],e.bezier2.y[0]);
		const Int32x2 ax0 = int32x2Sub (x0,context->centerA);
		const Int32x2 bx0 = int32x2Sub (x0,context->centerB);

		const Int32x2 x1 = int32x2(e.bezier2.x[1],e.bezier2.y[1]);
		const Int32x2 x2 = int32x2(e.bezier2.x[2],e.bezier2.y[2]);
		const Int32x2 ax2 = int32x2Sub (x2,context->centerA);
		const Int32x2 bx2 = int32x2Sub (x2,context->centerB);

		const Int32   ar0 = int32x2Abs (ax0);
		const Int32   ar2 = int32x2Abs (ax2);
		const Int32x2 ae0 = int32x2ScaleTo1_en (EN,ax0);	// radius unit vector
		const Int32x2 ae2 = int32x2ScaleTo1_en (EN,ax2);	// radius unit vector

		const Int32   br0 = int32x2Abs (bx0);
		const Int32   br2 = int32x2Abs (bx2);
		const Int32x2 be0 = int32x2ScaleTo1_en (EN,bx0);	// radius unit vector
		const Int32x2 be2 = int32x2ScaleTo1_en (EN,bx2);	// radius unit vector

		const Int32x2 v0 = int32x2MulExp2 (int32x2Sub (x1,x0),1);		// Bezier cartesian speed
		const Int32x2 v2 = int32x2MulExp2 (int32x2Sub (x2,x1),1);		// Bezier cartesian speed

		const Int32 av0Value = int32x2MulScalar_en (EN,ae0,v0);
		const Int32 bv0Value = int32x2MulScalar_en (EN,be0,v0);
		const Int32 av2Value = int32x2MulScalar_en (EN,ae2,v2);
		const Int32 bv2Value = int32x2MulScalar_en (EN,be2,v2);

		const Int32 sineLimit_en = (1<<EN) / 2048;		// :o) needs to be well chosen!!

		const Int32x2 ell0 = int32x2 (ar0,br0);
		const Int32x2 ell2 = int32x2 (ar2,br2);
		const Int32x2 ellv0 = int32x2 (av0Value,bv0Value);
		const Int32x2 ellv2 = int32x2 (av2Value,bv2Value);

		const Int32x2 ell1 = int32x2IntersectLineLineBetween_en (EN, sineLimit_en, ell0,ellv0, ell2,ellv2);

		return btElementBezier2 (
			int32Mul_en (EN, int32x2Abs(ellv0), int32Div_en (EN, e.v0,int32x2Abs (v0))),
			int32Mul_en (EN, int32x2Abs(ellv2), int32Div_en (EN, e.v1,int32x2Abs (v2))),
			e.level, ell0,ell1,ell2
		);
	} break;

	default:	return e;

	}
}

BtElement btElementEllipticToCartesian (BtEllipticContext *context, BtElement element) {
	switch (element.type) {
		case BT_TYPE_LINE:
		case BT_TYPE_BEZIER2: {
			const Int32x2 zero = int32x2MulExp2 (int32x2Add (context->centerA,context->centerB),-1);
			const Int32x2 ab = int32x2Sub (context->centerB,context->centerA);
			const Int32x2 ex = int32x2ScaleTo1_en (EN,ab);
			const Int32x2 ey = int32x2j (ex);

			const BtCurveTangent t0 = btElementCurveTangentBegin (element);
			const BtCurveTangent t1 = btElementCurveTangentEnd (element);
			const Int32 r0 = t0.x.x;
			const Int32 s0 = t0.x.y;
			const Int32 r1 = t1.x.x;
			const Int32 s1 = t1.x.y;

			const Int32 e = int32x2Abs(ab) / 2;
			const Int32 x0 = int64Div ((Int64)r0 *r0 - (Int64)s0 *s0,e*4);
			const Int32 y0 = uint64SqrtFloor ((Int64)r0 *r0 - (Int64)(x0+e) * (x0+e));
			const Int32x2 p0 = int32x2Add3 (zero, int32x2Scale_en (EN,ey,y0), int32x2Scale_en (EN,ex,x0));

			const Int32 x1 = int64Div ((Int64)r1 *r1 - (Int64)s1 *s1,e*4);
			const Int32 y1 = uint64SqrtFloor ((Int64)r1 *r1 - (Int64)(x1+e) * (x1+e));
			const Int32x2 p2 = int32x2Add3 (zero, int32x2Scale_en (EN,ey,y1), int32x2Scale_en (EN,ex,x1));

			const Int32x2 er0 = int32x2ScaleTo1_en (EN, int32x2Sub (p0,context->centerA)); 
			const Int32x2 es0 = int32x2ScaleTo1_en (EN, int32x2Sub (p0,context->centerB));
			// cartesian direction
			const Int32x2 v0 = int32x2Add (
				int32x2Scale_en (EN, er0, t0.t.x),
				int32x2Scale_en (EN, es0, t0.t.y)
			);

			const Int32x2 er2 = int32x2ScaleTo1_en (EN, int32x2Sub (p2,context->centerA)); 
			const Int32x2 es2 = int32x2ScaleTo1_en (EN, int32x2Sub (p2,context->centerB)); 
			// cartesian direction
			const Int32x2 v2 = int32x2Add (
				int32x2Scale_en (EN, er2, t1.t.x),
				int32x2Scale_en (EN, es2, t1.t.y)
			);
			Int32x2 p1;
			if (int32x2IntersectLineLine_en (EN, p0,v0, p2,v2, &p1)) return btElementBezier2 (
				int32Mul_en (EN, int32x2Abs(v0), int32Div_en (EN, element.v0,int32x2Abs (t0.t))),
				int32Mul_en (EN, int32x2Abs(v2), int32Div_en (EN, element.v1,int32x2Abs (t0.t))),
				element.level, p0,p1,p2
			);
			//else return btElementComment ("Elliptic to Cartesian failed.");
			else return btElementUndefined ();
		}
		break;

		default: return element;
	}

}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Parsing BT
//

bool btParseInt32_e20 (Fifo *fifo, Int32* value) {
	Fifo clone = *fifo;
	if (fifoParseExactString (&clone,"0x") && fifoParseHex (&clone, (Uint32*)value)) {
		fifoCopyReadPosition (fifo,&clone);
		return true;
	}
	else return fifoParseFixedPointInt (fifo,value,E20);
}

Int32x2 transformXy (const BtParserContext *ctx, Int32x2 p_e20) {
	Int32x2 q_e20 = int32x2Add (p_e20,ctx->translation_e20);
	q_e20 = int32x2ScaleXy_en (20,q_e20,ctx->scale_e20);
	if (ctx->rotation_e20!=0) q_e20 = int32x2MulComplex_en (20,q_e20, int32x2Rot_en (20,ctx->rotation_e20<<12));

	q_e20 = int32x2Add (q_e20,ctx->initialTranslation_e20);
	q_e20 = int32x2ScaleXy_en (20,q_e20,ctx->initialScale_e20);
	if (ctx->initialRotation_e20!=0) q_e20 = int32x2MulComplex_en (20,q_e20, int32x2Rot_en (20,ctx->initialRotation_e20<<12));

	return q_e20;
}

/*
Int32 transformY (const BtParserContext *context, Int32 value) {
	return (((Int64)value+context->ty) * context->scaleY_e20) >> 20;
}
*/

bool btErrorHere (BtParserContext *ctx, const char *msg, Fifo *dump) {
	fifoPrintString (ctx->errorOutput,ctx->fileName);
	fifoPrintChar (ctx->errorOutput,':');
	fifoPrintInt (ctx->errorOutput,ctx->lineNr);
	fifoPrintChar (ctx->errorOutput,':');
	fifoPrintString (ctx->errorOutput,msg);
	fifoPrintString (ctx->errorOutput," at this point -->");
	fifoPutFifo (ctx->errorOutput,dump);
	fifoPrintString (ctx->errorOutput,"<--\n");
	return false;
}

bool btErrorHereListOverflow (BtParserContext *ctx, Fifo *dump) {
	return btErrorHere (ctx,"Internal error: output list overflow",dump);
}

/** Parses one line. If you add additional commands to the language, then check the line for non-emptyness and parse after return from
 * this function.
 * @param line the input
 * @param list the output list of BtElements
 * @return true, if line was without error (so far, but maybe not yet processed command), false if an error happened.
 */
bool btParseLine (Fifo *line, BtList *list, BtParserContext *ctx) {
	ctx->lineNr ++;
	fifoParseBlanks (line);
	if (!fifoCanRead (line)			// empty line considered a comment
	|| fifoParseExactChar (line,'#')) {	// comment
		fifoParseExactChar (line,' ');		// remove one space
		const char * comment = stringTableWriteFifoUnique (ctx->stringTable,line);
		fifoReset (line);	// consume all
		if (comment!=0) {
			BtElement element = {
				.type = BT_TYPE_COMMENT,
				.v0 = ctx->v0,
				.v1 = ctx->v0,		// doesn't change speed
				.level = ctx->level,
				{	.comment = { comment
					},
				}
			};

			if (btListCanWrite (list)) {
				btListWrite (list,element);
			}
			else return btErrorHereListOverflow (ctx,line);
		}
		else return btErrorHere (ctx,"Internal error: string table overflow",line);
	}
	else if (fifoParseExactString (line,"inline")) {
		fifoParseBlanks(line);
		int format = 0;
		if (fifoParseExactString (line,outputFormatSymbols[OUTPUT_FORMAT_GCODE])) format = OUTPUT_FORMAT_GCODE;
		else if (fifoParseExactString (line,outputFormatSymbols[OUTPUT_FORMAT_PS_RAYS])) format = OUTPUT_FORMAT_PS_RAYS;
		else if (fifoParseExactString (line,outputFormatSymbols[OUTPUT_FORMAT_PS])) format = OUTPUT_FORMAT_PS;
		else if (fifoParseExactString (line,outputFormatSymbols[OUTPUT_FORMAT_B2C])) format = OUTPUT_FORMAT_B2C;
		else return btErrorHere (ctx,"inline format not supported",line);
		fifoParseBlanks(line);
		const char* text = stringTableWriteFifoUnique (ctx->stringTable,line);
		fifoReset (line);	// consume all
		if (text!=0) {
			if (btListCanWrite (list)) {
				btListWrite (list,btElementInlined (format,text));
			}
			else return btErrorHereListOverflow (ctx,line);
		}
		else return btErrorHere (ctx,"Internal error: string table overflow",line);
	}
	else if (fifoParseExactString (line,"label")) {
		fifoParseBlanks(line);
		const char* text = stringTableWriteFifoUnique (ctx->stringTable,line);
		fifoReset (line);	// consume all
		if (text!=0) {
			BtElement element = {
				.type = BT_TYPE_LABEL,
				.v0 = ctx->v0,
				.v1 = ctx->v0,
				.level = ctx->level,
				{
					.label = { .text = text },
				},
			};
			if (btListCanWrite (list)) {
				btListWrite (list,element);
			}
			else return btErrorHereListOverflow (ctx,line);
		}
		else return btErrorHere (ctx,"Internal error: string table overflow",line);
	}
	else if (fifoParseExactString (line,"level")) {
		fifoParseBlanks(line);
		int level;
		if (fifoParseInt (line,&level)) {
			ctx->level = level;
		}
		else return btErrorHere (ctx,"integer level expected",line);
	}
	else if (fifoParseExactString (line,"end")) {	// maybe we should remove (useless) end?
	}
	else if (fifoParseExactString (line,"scale")) {
		fifoParseBlanks (line);
		Int32 scaleX,scaleY;
		if (btParseInt32_e20 (line,&scaleX)) {
			ctx->scale_e20.x = scaleX;
			ctx->scale_e20.y = scaleX;
			fifoParseBlanks (line);
			if (fifoCanRead (line)) {
				if (btParseInt32_e20 (line,&scaleY)) {
					ctx->scale_e20.y = scaleY;
				}
				else return btErrorHere (ctx,"scaleY factor expected",line);
			}
		}
		else return btErrorHere (ctx,"scale factor(s) expected",line);
	}
	else if (fifoParseExactString (line,"rotate")) {
		fifoParseBlanks (line);
		if (btParseInt32_e20 (line,&ctx->rotation_e20)) {
			// fine
		}
	}
	else if (fifoParseExactString (line,"speedto")) {
		fifoParseBlanks (line);
		if (btParseInt32_e20 (line,&ctx->v1));	// new v1
		else return btErrorHere (ctx,"speed v1 expected",line);
	}
	else if (fifoParseExactString (line,"speed")) {
		fifoParseBlanks (line);
		if (btParseInt32_e20 (line,&ctx->v0)) {
			if (fifoParseBlanks (line) && btParseInt32_e20 (line,&ctx->v1));	// fine. both set.
			else if (!fifoCanRead (line)) ctx->v1 = ctx->v0;		// v0 provided, only ==> v1=v0
			else return btErrorHere (ctx,"speed v1 expected",line);
		}
		else return btErrorHere (ctx,"speeds v0 v1 expected",line);
	}
	else if (fifoParseExactString (line,"translate")) {
		fifoParseBlanks (line);
		Int32 tx,ty;
		if (btParseInt32_e20 (line,&tx)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&ty)) {
			ctx->translation_e20.x = tx;
			ctx->translation_e20.y = ty;
			//btParserContextComputeMatrix (ctx);
		}
		else return btErrorHere (ctx,"x y position expected",line);
	}
	else if (fifoParseExactString (line,"origin")) {
		fifoParseBlanks (line);
		Int32x2 p;
		if (btParseInt32_e20 (line,&p.x)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&p.y)) {
			ctx->p0 = transformXy(ctx,p);
		}
		else return btErrorHere (ctx,"x y position expected",line);
	}
	else if (fifoParseExactString (line,"lineto")) {
		if (!int32x2IsDefined (ctx->p0)) return btErrorHere (ctx,"current point still undefined",line);
		fifoParseBlanks (line);
		Int32x2 p;
		if (btParseInt32_e20 (line,&p.x)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&p.y)) {
			const Int32x2 p1 = transformXy (ctx,p);
			BtElement element = {
				.type = BT_TYPE_LINE,
				.v0 = ctx->v0,
				.v1 = ctx->v1,
				.level = ctx->level,
				{	.line = {
						.x = { ctx->p0.x, p1.x	},
						.y = { ctx->p0.y, p1.y	}
					},
				}
			};
			ctx->p0 = p1;
			ctx->v0 = ctx->v1;

			if (btListCanWrite (list)) {
				btListWrite (list,element);
			}
			else return btErrorHereListOverflow (ctx,line);
			//return true;
		}
		else return btErrorHere (ctx,"x y position expected",line);
	}
	else if (fifoParseExactString (line,"bezier2arcto")) {	// for easy reading of G-Code: 'center' and destination point
		if (!int32x2IsDefined (ctx->p0)) return btErrorHere (ctx,"current point still undefined",line);
		fifoParseBlanks (line);
		Int32x2 _c,_p2;
		if (btParseInt32_e20 (line,&_c.x)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&_c.y)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&_p2.x)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&_p2.y)) {	// emulate bezier2 through bezier3
			const Int32x2 c = transformXy (ctx,_c);
			const Int32x2 p2 = transformXy (ctx,_p2);
			const Int32x2 dir0 = int32x2j (int32x2ScaleTo1_en (EN,int32x2Sub (ctx->p0,c)));	// scaling to improve numeric stability
			const Int32x2 dir2 = int32x2j (int32x2ScaleTo1_en (EN,int32x2Sub (p2,c)));	// scaling to improve numeric stability
			Int32x2 p1;
			if (int32x2IntersectLineLine_en (EN,ctx->p0,dir0,p2,dir2,&p1)) {
				BtElement element = {
					.type = BT_TYPE_BEZIER2,
					.v0 = ctx->v0,
					.v1 = ctx->v1,
					.level = ctx->level,
					{	.bezier2 = {
							.x = { ctx->p0.x, p1.x, p2.x	},
							.y = { ctx->p0.y, p1.y, p2.y	},
						},
					}
				};
				ctx->p0 = p2;
				ctx->v0 = ctx->v1;

				if (btListCanWrite (list)) {
					btListWrite (list,element);
				}
				else return btErrorHereListOverflow (ctx,line);
			}
			else return btErrorHere (ctx,"bezier2arcto: cannot construct intersection",line);
		}
		else return btErrorHere (ctx,"x1 y1 x2 y2 positions expected",line);
	}
	else if (fifoParseExactString (line,"bezier2to")) {
		if (!int32x2IsDefined (ctx->p0)) return btErrorHere (ctx,"current point still undefined",line);
		fifoParseBlanks (line);
		Int32x2 _p1,_p2;
		if (btParseInt32_e20 (line,&_p1.x)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&_p1.y)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&_p2.x)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&_p2.y)) {	// emulate bezier2 through bezier3
			const Int32x2 p1 = transformXy (ctx,_p1);
			const Int32x2 p2 = transformXy (ctx,_p2);
			BtElement element = {
				.type = BT_TYPE_BEZIER2,
				.v0 = ctx->v0,
				.v1 = ctx->v1,
				.level = ctx->level,
				{	.bezier2 = {
						.x = { ctx->p0.x, p1.x, p2.x	},
						.y = { ctx->p0.y, p1.y, p2.y	},
					},
				}
			};
			ctx->p0 = p2;
			ctx->v0 = ctx->v1;

			if (btListCanWrite (list)) {
				btListWrite (list,element);
			}
			else return btErrorHereListOverflow (ctx,line);
			//return true;
		}
		else return btErrorHere (ctx,"x1 y1 x2 y2 positions expected",line);
	}
	else if (fifoParseExactString (line,"bezier3to")) {
		if (!int32x2IsDefined (ctx->p0)) return btErrorHere (ctx,"current point still undefined",line);
		fifoParseBlanks (line);
		Int32x2 _p1,_p2,_p3;
		if (btParseInt32_e20 (line,&_p1.x)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&_p1.y)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&_p2.x)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&_p2.y)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&_p3.x)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&_p3.y)) {	// approximate bezier3 through bezier2
			const Int32x2 p1 = transformXy (ctx,_p1);
			const Int32x2 p2 = transformXy (ctx,_p2);
			const Int32x2 p3 = transformXy (ctx,_p3);
			const Int32x2 v0 = int32x2Sub (p1,ctx->p0);
			const Int32x2 v2 = int32x2Sub (p3,p2);
			Int32x2 b2p1;
			if (int32x2IntersectLineLine_en (EN,
				ctx->p0, int32x2ScaleTo1_en (EN,v0),
				p3, int32x2ScaleTo1_en (EN,v2),
				&b2p1)) {

				BtElement element = {
					.type = BT_TYPE_BEZIER2,
					.v0 = ctx->v0,
					.v1 = ctx->v1,
					.level = ctx->level,
					{	.bezier2 = {
							.x = { ctx->p0.x, b2p1.x, p3.x	},
							.y = { ctx->p0.y, b2p1.y, p3.y	},
						},
					}
				};
				ctx->p0 = p3;
				ctx->v0 = ctx->v1;

				if (btListCanWrite (list)) {
					btListWrite (list,element);
				}
				else return btErrorHereListOverflow (ctx,line);
			}
			else return btErrorHere (ctx,"Cannot approximate this Bezier3 curve through Bezier2.",line);
		}
		else return btErrorHere (ctx,"x1 y1 x2 y2 positions expected",line);
	}
	// sbt commands
	else if (fifoParseExactString (line,"line")) {
		fifoParseBlanks (line);
		Int32x2 p0,p1;
		Int32 level,v0,v1;
		if (btParseInt32_e20 (line,&p0.x)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&p0.y)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&p1.x)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&p1.y)
		&& (fifoParseBlanks (line),true)
		&& fifoParseExactString (line,"level")
		&& (fifoParseBlanks (line),true)
		&& fifoParseInt (line,&level)
		&& (fifoParseBlanks (line),true)
		&& fifoParseExactString (line,"v0")
		&& (fifoParseBlanks (line),true)
		&& btParseInt32_e20 (line,&v0)
		&& (fifoParseBlanks (line),true)
		&& fifoParseExactString (line,"v1")
		&& (fifoParseBlanks (line),true)
		&& btParseInt32_e20 (line,&v1)
		) {
			const Int32x2 x0 = transformXy (ctx,p0);
			const Int32x2 x1 = transformXy (ctx,p1);
			BtElement element = {
				.type = BT_TYPE_LINE,
				.v0 = v0,
				.v1 = v1,
				.level = ctx->level,
				{	.line = {
						.x = { x0.x, x1.x	},
						.y = { x0.y, x1.y	}
					},
				}
			};
			ctx->p0 = x1;
			ctx->v0 = v1;
			ctx->v1 = v1;

			if (btListCanWrite (list)) {
				btListWrite (list,element);
			}
			else return btErrorHereListOverflow (ctx,line);
			//return true;
		}
		else return btErrorHere (ctx,"x/y position pair expected",line);
	}

	else if (fifoParseExactString (line,"bezier2")) {
		fifoParseBlanks (line);
		Int32x2 p0,p1,p2;
		Int32 level,v0,v1;
		if (btParseInt32_e20 (line,&p0.x)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&p0.y)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&p1.x)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&p1.y)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&p2.x)
		&& fifoParseBlanks (line)
		&& btParseInt32_e20 (line,&p2.y)
		&& (fifoParseBlanks (line),true)
		&& fifoParseExactString (line,"level")
		&& (fifoParseBlanks (line),true)
		&& fifoParseInt (line,&level)
		&& (fifoParseBlanks (line),true)
		&& fifoParseExactString (line,"v0")
		&& (fifoParseBlanks (line),true)
		&& btParseInt32_e20 (line,&v0)
		&& (fifoParseBlanks (line),true)
		&& fifoParseExactString (line,"v1")
		&& (fifoParseBlanks (line),true)
		&& btParseInt32_e20 (line,&v1)
		) {
			const Int32x2 x0 = transformXy (ctx,p0);
			const Int32x2 x1 = transformXy (ctx,p1);
			const Int32x2 x2 = transformXy (ctx,p2);
			BtElement element = {
				.type = BT_TYPE_BEZIER2,
				.v0 = v0,
				.v1 = v1,
				.level = ctx->level,
				{	.bezier2 = {
						.x = { x0.x, x1.x, x2.x	},
						.y = { x0.y, x1.y, x2.y	}
					},
				}
			};
			ctx->p0 = x1;
			ctx->v0 = v1;
			ctx->v1 = v1;

			if (btListCanWrite (list)) {
				btListWrite (list,element);
			}
			else return btErrorHereListOverflow (ctx,line);
			//return true;
		}
		else return btErrorHere (ctx,"x/y position triple expected",line);
	}

	else return true; //return btErrorHere ("Syntax error",line,ctx);

	// match, but trailing characters left
	if (fifoCanRead (line)) btErrorHere (ctx,"Trailing characters on line (ignored):",line);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// BT language output

bool btBezierToolPrintValue (BezierToolContext *ctx, Int32 value_e20) {
	if (ctx->hex) return fifoPrintString (ctx->output,"0x") && fifoPrintHex (ctx->output, value_e20, 8,8);
	//else return fifoPrintFixedPointInt (ctx->output, value_e20 ,E20, 1,11,7,false);
	else return fifoPrintInt32Easy_e20 (ctx->output, value_e20,7);
}

bool btBezierToolPrintXy (BezierToolContext *ctx, Int32 x_e20, Int32 y_e20) {
	return	btBezierToolPrintValue (ctx,x_e20)
		&& fifoPrintChar (ctx->output,' ')
		&& btBezierToolPrintValue (ctx,y_e20);
}

bool btBezierToolBegin (BezierToolContext *ctx) {
	//const char *btHeader = "scale 1\n";
	ctx->level = BT_INVALID_LEVEL;
	ctx->p0 = int32x2Undefined();
	ctx->v0 = INT32_UNDEFINED;
	//return fifoPrintString (ctx->output,btHeader);
	return true;	// be silent
}

bool btBezierToolElement (BezierToolContext *ctx, BtElement e) {
	// stroke and level changes...
	if (e.level!=ctx->level) {	// new level
		fifoPrintString (ctx->output,"level ");
		fifoPrintInt (ctx->output,e.level);
		fifoPrintString (ctx->output,"\n");
		ctx->level = e.level;
	}

	// check for speed updates
	switch(e.type) {
		case BT_TYPE_LINE:
		case BT_TYPE_BEZIER2:
			if (ctx->v0!=e.v0) {		// speed leap
				fifoPrintString (ctx->output,"speed ");
				if (e.v0!=e.v1) btBezierToolPrintXy (ctx,e.v0,e.v1);	// dual value output
				else btBezierToolPrintValue (ctx,e.v0);			// single value output
				fifoPrintLf (ctx->output);
			}
			else if (ctx->v0!=e.v1) {	// contiguous
				fifoPrintString (ctx->output,"speedto ");
				btBezierToolPrintValue (ctx,e.v1);
				fifoPrintLf (ctx->output);
			}
			else ;
			break;
		default: ;	// no speed statement neccessary	
	}

	switch (e.type) {
		case BT_TYPE_LINE:
			if (e.line.x[0]!=ctx->p0.x || e.line.y[0]!=ctx->p0.y) {
				fifoPrintString (ctx->output,"origin ");
				btBezierToolPrintXy (ctx,e.line.x[0],e.line.y[0]);
				fifoPrintString (ctx->output,"\n");
			}
			ctx->p0 = int32x2 (e.line.x[1],e.line.y[1]);
			ctx->v0 = e.v1;
			fifoPrintString (ctx->output,"lineto ");
			btBezierToolPrintXy (ctx,e.line.x[1],e.line.y[1]);
			return fifoPrintString (ctx->output,"\n");
		break;

		case BT_TYPE_BEZIER2: {
			if (e.bezier2.x[0]!=ctx->p0.x || e.bezier2.y[0]!=ctx->p0.y) {
				fifoPrintString (ctx->output,"origin ");
				btBezierToolPrintXy (ctx,e.bezier2.x[0],e.bezier2.y[0]);
				fifoPrintString (ctx->output,"\n");
			}
			ctx->p0 = int32x2 (e.bezier2.x[2],e.bezier2.y[2]);
			ctx->v0 = e.v1;

			fifoPrintString (ctx->output,"bezier2to ");
			btBezierToolPrintXy (ctx,e.bezier2.x[1],e.bezier2.y[1]);
			fifoPrintString (ctx->output,"  ");
			btBezierToolPrintXy (ctx,e.bezier2.x[2],e.bezier2.y[2]);
			return fifoPrintString (ctx->output,"\n");
		} break;

		case BT_TYPE_LABEL:
			fifoPrintString (ctx->output,"label ");
			fifoPrintString (ctx->output,e.label.text);
			return fifoPrintString (ctx->output,"\n");
		break;

		case BT_TYPE_COMMENT:
			return ctx->noComments || true
			&& fifoPrintString (ctx->output,"# ")
			&& fifoPrintString (ctx->output,e.comment.text)
			&& fifoPrintString (ctx->output,"\n");
		break;

		case BT_TYPE_INLINED:
			return true
			&& fifoPrintString (ctx->output,"inline ")
			&& fifoPrintString (ctx->output,outputFormatSymbols[e.inlined.format])
			&& fifoPrintSpace (ctx->output)
			&& fifoPrintString (ctx->output,e.inlined.text)
			&& fifoPrintString (ctx->output,"\n");
		break;

		default: return false; 	// unknown element type
			// fprintf (stderr,"Invalid BT type.\n");
	}
}

bool btBezierToolDone (BezierToolContext *ctx) {
//	return fifoPrintString (ctx->output,"end\n");
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Stateless BT language output

bool btStatelessBezierToolPrintValue (StatelessBezierToolContext *ctx, Int32 value_e20) {
	if (ctx->hex) return fifoPrintString (ctx->output,"0x") && fifoPrintHex (ctx->output, value_e20, 8,8);
	else return fifoPrintInt32Easy_e20 (ctx->output, value_e20,7);
}

bool btStatelessBezierToolPrintXy (StatelessBezierToolContext *ctx, Int32 x_e20, Int32 y_e20) {
	return	btStatelessBezierToolPrintValue (ctx,x_e20)
		&& fifoPrintChar (ctx->output,' ')
		&& btStatelessBezierToolPrintValue (ctx,y_e20);
}

bool btStatelessBezierToolPrintState (StatelessBezierToolContext *ctx, BtElement e) {
	return	fifoPrintString (ctx->output,"level ")
		&& fifoPrintInt (ctx->output,e.level)
		&& fifoPrintString (ctx->output," v0 ")
		&& btStatelessBezierToolPrintValue (ctx,e.v0)
		&& fifoPrintString (ctx->output," v1 ")
		&& btStatelessBezierToolPrintValue (ctx,e.v1)
		;
}

bool btStatelessBezierToolElement (StatelessBezierToolContext *ctx, BtElement e) {
	switch (e.type) {
		case BT_TYPE_LINE:
			fifoPrintString (ctx->output,"line ");
			btStatelessBezierToolPrintXy (ctx,e.line.x[0],e.line.y[0]);
			fifoPrintSpace (ctx->output);
			fifoPrintSpace (ctx->output);
			btStatelessBezierToolPrintXy (ctx,e.line.x[1],e.line.y[1]);
			fifoPrintSpace (ctx->output);
			fifoPrintSpace (ctx->output);
			btStatelessBezierToolPrintState (ctx,e);
			return fifoPrintLf (ctx->output);
		break;

		case BT_TYPE_BEZIER2: {
			fifoPrintString (ctx->output,"bezier2 ");
			btStatelessBezierToolPrintXy (ctx,e.bezier2.x[0],e.bezier2.y[0]);
			fifoPrintSpace (ctx->output);
			fifoPrintSpace (ctx->output);
			btStatelessBezierToolPrintXy (ctx,e.bezier2.x[1],e.bezier2.y[1]);
			fifoPrintSpace (ctx->output);
			fifoPrintSpace (ctx->output);
			btStatelessBezierToolPrintXy (ctx,e.bezier2.x[2],e.bezier2.y[2]);
			fifoPrintSpace (ctx->output);
			fifoPrintSpace (ctx->output);
			btStatelessBezierToolPrintState (ctx,e);
			return fifoPrintLf (ctx->output);
		} break;

		case BT_TYPE_LABEL:
			fifoPrintString (ctx->output,"label ");
			fifoPrintString (ctx->output,e.label.text);
			return fifoPrintLf (ctx->output);
		break;

		case BT_TYPE_COMMENT:
			return ctx->noComments || true
			&& fifoPrintString (ctx->output,"# ")
			&& fifoPrintString (ctx->output,e.comment.text)
			&& fifoPrintLf (ctx->output);
		break;

		case BT_TYPE_INLINED:
			return true
			&& fifoPrintString (ctx->output,"inline ")
			&& fifoPrintString (ctx->output,outputFormatSymbols[e.inlined.format])
			&& fifoPrintSpace (ctx->output)
			&& fifoPrintString (ctx->output,e.inlined.text)
			&& fifoPrintString (ctx->output,"\n");
		break;

		default: return false; 	// unknown element type
			// fprintf (stderr,"Invalid BT type.\n");
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Postscript output

bool btPostscriptPrintValue (BtPostscriptContext *ctx, Int32 value_e20) {
	//return fifoPrintFixedPointInt (ctx->output, value_e20 ,E20, 1,11,7,false);
	return fifoPrintInt32Easy_e20 (ctx->output, value_e20,7);
}

bool btPostscriptPrintXy (BtPostscriptContext *ctx, Int32 x_e20, Int32 y_e20) {
	return	btPostscriptPrintValue (ctx,x_e20)
		&& fifoPrintChar (ctx->output,' ')
		&& btPostscriptPrintValue (ctx,y_e20);
}

bool btPostscriptBegin (BtPostscriptContext *ctx) {
	ctx->p0 = int32x2Undefined();
	ctx->level = BT_INVALID_LEVEL;
	bool success = fifoPrintString (ctx->output, "%!PS-Adobe-2.0\n%%Creator: bt (BezierTool)\n");
	if (!ctx->noHeaders) success = success
		&& fifoPrintString (ctx->output,"/level { pop } def % empty definition\n72 25.4 div dup scale\n")
		&& btPostscriptPrintXy (ctx, ctx->pageOrigin.x, ctx->pageOrigin.y) && fifoPrintString (ctx->output," translate % offset in page\n")
		&& (ctx->pageZoom==E20 ||
			btPostscriptPrintValue (ctx, ctx->pageZoom) && fifoPrintString (ctx->output," dup scale % custom zoom\n")
		)
		&& (ctx->zoomOrigin.x==0 && ctx->zoomOrigin.y==0 ||	// zoom origin unchanged => omit additional translate
			btPostscriptPrintXy (ctx, -ctx->zoomOrigin.x, -ctx->zoomOrigin.y) && fifoPrintString (ctx->output," translate % zoom origin\n")
		)
		&& fifoPrintString (ctx->output,"0.2 setlinewidth\n\n")
		;
	if (ctx->coordinate!=0) {
		success = success
		&& fifoPrintString (ctx->output,
			"% coordinate system\n"
			"% fine grid\n"
			"0.7 setgray 0.1 setlinewidth\n"
			"newpath\n"
			"-100 1 100 { /x exch def x -150 moveto x 150 lineto stroke  } for\n"
			"-150 1 150 { /y exch def -100 y moveto 100 y lineto stroke  } for\n"
			"% coarse grid\n"
			"0.5 setgray 0.2 setlinewidth\n"
			"-100 10 100 { /x exch def x -150 moveto x 150 lineto stroke  } for\n"
			"-150 10 150 { /y exch def -100 y moveto 100 y lineto stroke  } for\n"
			"0.3 setgray\n"
			"% X/Y axis\n"
			"-100 0 moveto 100 0 lineto stroke\n"
			"0 -150 moveto 0 150 lineto stroke\n"
			//"/NewCenturySchlbook-Roman findfont 3 scalefont setfont\n"
			"/Times-Roman findfont 3 scalefont setfont\n"
			"% notches and numbers\n"
			"-100 10 100 { /x exch def x -1 moveto x 1 lineto stroke x 2 moveto x (    ) cvs show } for\n"
			"-150 10 150 { /y exch def -1 y moveto 1 y lineto stroke 2 y moveto y (    ) cvs show } for\n"
			"0 setgray\n"
		);
	}
	return success;
}

bool btPostscriptElementPath (BtPostscriptContext *ctx, BtElement e) {
	// stroke and level changes...
	if (e.level!=ctx->level) {		// new level
		if (ctx->inPath) {
			fifoPrintString (ctx->output,"stroke\n");	// stroke previous path first.
		}
		ctx->inPath = false;		// make sure, a moveto follows...;
		ctx->level = e.level;
		fifoPrintInt (ctx->output,e.level);
		fifoPrintString (ctx->output," level\n");
	}

	if (btElementIsPath(e) && !int32x2Eq (btElementPointBegin(e),ctx->p0)) {	// path element and different position
		if (ctx->inPath) {
			fifoPrintString (ctx->output,"stroke\n");	// stroke previous path first.
		}
		ctx->inPath = false;		// make sure, a moveto follows...
	}

	switch (e.type) {
		case BT_TYPE_LINE:
			if (!ctx->inPath) {
				fifoPrintString (ctx->output,"newpath ");
				btPostscriptPrintXy (ctx,e.line.x[0],e.line.y[0]);
				fifoPrintString (ctx->output," moveto\n");
			}
			ctx->inPath = true;
			btPostscriptPrintXy (ctx,e.line.x[1],e.line.y[1]);
			ctx->p0 = int32x2 (e.line.x[1],e.line.y[1]);
			return fifoPrintString (ctx->output," lineto\n");
		break;

		case BT_TYPE_BEZIER2: {
			if (!ctx->inPath) {
				fifoPrintString (ctx->output,"newpath ");
				btPostscriptPrintXy (ctx,e.bezier2.x[0],e.bezier2.y[0]);
				fifoPrintString (ctx->output," moveto\n");
			}
			ctx->inPath = true;
			// we have to emulate bezier2 with Postscript's bezier3
			const Int32 x31 = (e.bezier2.x[0] + (Int64)2*e.bezier2.x[1] ) / 3;
			const Int32 y31 = (e.bezier2.y[0] + (Int64)2*e.bezier2.y[1] ) / 3;
			const Int32 x32 = (e.bezier2.x[2] + (Int64)2*e.bezier2.x[1] ) / 3;
			const Int32 y32 = (e.bezier2.y[2] + (Int64)2*e.bezier2.y[1] ) / 3;
			btPostscriptPrintXy (ctx,x31,y31);
			fifoPrintChar (ctx->output,' ');
			btPostscriptPrintXy (ctx,x32,y32);
			fifoPrintChar (ctx->output,' ');
			btPostscriptPrintXy (ctx,e.bezier2.x[2],e.bezier2.y[2]);
			ctx->p0 = int32x2 (e.bezier2.x[2],e.bezier2.y[2]);
			return fifoPrintString (ctx->output," curveto\n");
		} break;

		case BT_TYPE_LABEL:
			if (!ctx->literalLabels) fifoPrintString (ctx->output,"% ");
			fifoPrintString (ctx->output,e.label.text);
			return fifoPrintString (ctx->output,"\n");
		break;

		case BT_TYPE_COMMENT:
			fifoPrintString (ctx->output,"% ");
			fifoPrintString (ctx->output,e.comment.text);
			return fifoPrintString (ctx->output,"\n");
		break;

		case BT_TYPE_INLINED:
			return e.inlined.format!=OUTPUT_FORMAT_PS
			|| fifoPrintString (ctx->output,e.inlined.text)
			&& fifoPrintLf (ctx->output);
		break;

		default: return false;
	}
}

bool btPostscriptElementRay (BtPostscriptContext *ctx, BtElement e) {
	// stroke and level changes...
	if (e.level!=ctx->level) {		// new level
		ctx->level = e.level;
		fifoPrintInt (ctx->output,e.level);
		fifoPrintString (ctx->output," level\n");
	}

	switch (e.type) {
		case BT_TYPE_LINE:
			fifoPrintString (ctx->output,"newpath ");
			btPostscriptPrintXy (ctx,ctx->origin.x,ctx->origin.y); fifoPrintString (ctx->output," moveto ");
			btPostscriptPrintXy (ctx,e.line.x[0],e.line.y[0]); fifoPrintString (ctx->output," lineto stroke\nnewpath ");
			btPostscriptPrintXy (ctx,ctx->origin.x,ctx->origin.y); fifoPrintString (ctx->output," moveto ");
			btPostscriptPrintXy (ctx,e.line.x[1],e.line.y[1]);
			return fifoPrintString (ctx->output," lineto stroke\n");
		break;

		case BT_TYPE_BEZIER2: {
			fifoPrintString (ctx->output,"gsave currentrgbcolor  3 { 0.3 mul 0.7 add 3 1 roll } repeat  setrgbcolor newpath ");
			btPostscriptPrintXy (ctx,ctx->origin.x,ctx->origin.y); fifoPrintString (ctx->output," moveto ");
			btPostscriptPrintXy (ctx,e.bezier2.x[1],e.bezier2.y[1]); fifoPrintString (ctx->output," lineto stroke grestore\nnewpath ");
			btPostscriptPrintXy (ctx,ctx->origin.x,ctx->origin.y); fifoPrintString (ctx->output," moveto ");
			btPostscriptPrintXy (ctx,e.bezier2.x[0],e.bezier2.y[0]); fifoPrintString (ctx->output," lineto stroke\nnewpath ");
			btPostscriptPrintXy (ctx,ctx->origin.x,ctx->origin.y); fifoPrintString (ctx->output," moveto ");
			btPostscriptPrintXy (ctx,e.bezier2.x[2],e.bezier2.y[2]);
			return fifoPrintString (ctx->output," lineto stroke\n");
		} break;

		case BT_TYPE_LABEL:
			if (!ctx->literalLabels) fifoPrintString (ctx->output,"% ");
			fifoPrintString (ctx->output,e.label.text);
			return fifoPrintString (ctx->output,"\n");
		break;

		case BT_TYPE_COMMENT:
			fifoPrintString (ctx->output,"% ");
			fifoPrintString (ctx->output,e.comment.text);
			return fifoPrintString (ctx->output,"\n");
		break;

		case BT_TYPE_INLINED:
			return e.inlined.format!=OUTPUT_FORMAT_PS_RAYS
			|| fifoPrintString (ctx->output,e.inlined.text)
			&& fifoPrintLf (ctx->output);
		break;

		default: return false;
	}
}

bool btPostscriptElement (BtPostscriptContext *ctx, BtElement e) {
	switch (ctx->format) {
		case OUTPUT_FORMAT_PS_RAYS:	return btPostscriptElementRay (ctx,e);
		default:			return btPostscriptElementPath (ctx,e);
	}
}

bool btPostscriptDone (BtPostscriptContext *ctx) {
	if (ctx->inPath) fifoPrintString (ctx->output,"stroke\n");
	return ctx->noFooters || fifoPrintString (ctx->output,"showpage\n");
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// G-Code output

bool btGcodePrintValue (BtGcodeContext *ctx, Int32 value_e20) {
	//return fifoPrintFixedPointInt (ctx->output, value_e20 ,E20, 1,11,4,false);	// print to um
	return fifoPrintInt32Easy_e20 (ctx->output, value_e20,4);
}

bool btGcodePrintAxis (BtGcodeContext *ctx, int axis, Int32 value_e20) {
	return	fifoPrintChar (ctx->output,ctx->axes[axis&1])
		//&& fifoPrintInt32_e20 (ctx->output, value_e20 1,4,false);	// print to um
		&& fifoPrintInt32Easy_e20 (ctx->output, value_e20 ,4);	// print to um
}

bool btGcodePrintXy (BtGcodeContext *ctx, Int32 x_e20, Int32 y_e20) {
	return	true
		&& btGcodePrintAxis (ctx,0,x_e20)
		&& fifoPrintChar (ctx->output,' ')
		&& btGcodePrintAxis (ctx,1,y_e20)
		;
}

bool btGcodeBegin (BtGcodeContext *ctx) {
	return	fifoPrintString (ctx->output,"; Generated by bt (BezierTool)\n")
		&& (ctx->noHeaders || fifoPrintString (ctx->output,
			"G71 ; dimesions in mm\n"
			"G64 ; continuous path mode\n\n"
		));
}

bool btGcodeN (BtGcodeContext *ctx, bool print) {
	ctx->n += 10;
	if (print) {
		return	fifoPrintChar (ctx->output,'N')
			&& fifoPrintInt (ctx->output,ctx->n)
			&& fifoPrintChar (ctx->output,' ');
	}
	else return true;
}

bool btGcodeElement (BtGcodeContext *ctx, BtElement e) {

	// stroke and level changes...
	if (e.level!=ctx->level) {	// new level
		ctx->level = e.level;
		btGcodeN (ctx,true);
		fifoPrintString (ctx->output, e.level ? "G01" : "G0");
		fifoPrintString (ctx->output,"\n");
	}

	switch (e.type) {
		case BT_TYPE_LINE:
			btGcodeN (ctx,true);
			if (!ctx->inPath) {
				btGcodePrintXy (ctx,e.line.x[0],e.line.y[0]);
				fifoPrintChar (ctx->output,'\n');
				btGcodeN (ctx,true);
				ctx->inPath = true;
			}
			btGcodePrintXy (ctx,e.line.x[1],e.line.y[1]);
			return fifoPrintChar (ctx->output,'\n');
		break;

		case BT_TYPE_BEZIER2: {
			btGcodeN (ctx,true);
			if (!ctx->inPath) {
				btGcodePrintXy (ctx,e.bezier2.x[0],e.bezier2.y[0]);
				fifoPrintChar (ctx->output,'\n');
				btGcodeN (ctx,true);
				ctx->inPath = true;
			}
			btGcodePrintXy (ctx,e.bezier2.x[1],e.bezier2.y[1]);
			fifoPrintChar (ctx->output,'\n');
			btGcodeN (ctx,true);
			btGcodePrintXy (ctx,e.bezier2.x[2],e.bezier2.y[2]);
			return fifoPrintChar (ctx->output,'\n');
		} break;

		case BT_TYPE_LABEL:
			btGcodeN (ctx,ctx->literalLabels);
			if (!ctx->literalLabels) fifoPrintString (ctx->output,"; ");
			fifoPrintString (ctx->output,e.label.text);
			return fifoPrintString (ctx->output,"\n");
		break;

		case BT_TYPE_COMMENT:
			fifoPrintString (ctx->output,"; ");
			fifoPrintString (ctx->output,e.comment.text);
			return fifoPrintString (ctx->output,"\n");
		break;

		case BT_TYPE_INLINED:
			return e.inlined.format!=OUTPUT_FORMAT_GCODE
			|| fifoPrintString (ctx->output,e.inlined.text)
			&& fifoPrintLf (ctx->output);
		break;

		default: return false;
			
	}
}

bool btGcodeDone (BtGcodeContext *ctx) {
	return fifoPrintString (ctx->output,"M0 ; stop\n");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// FC190 Bezier2Command output (array)


bool btB2cPrintValue (BtB2cContext *ctx, Int32 value_e20) {
	bool success = fifoPrintString (ctx->output,"0x") && fifoPrintHex (ctx->output, value_e20, 8,8);
	if (!ctx->hex) success = success
		&& fifoPrintString (ctx->output,"/*")
		&& fifoPrintInt32Easy_e20 (ctx->output, value_e20,7)
		&& fifoPrintString (ctx->output,"_e20*/")
		;
	return success;
}

bool btB2cBegin (BtB2cContext *ctx) {
	return fifoPrintString (ctx->output,"// Generated by BezierTool -fb2c\n")
	&& (ctx->noHeaders || true
		&& fifoPrintString (ctx->output,"const Bezier2Command ")
		&& fifoPrintString (ctx->output,ctx->name)
		&& fifoPrintString (ctx->output,"[] = {\n")
	);
}

bool btB2cElement (BtB2cContext *ctx, BtElement e) {
	switch (e.type) {
		case BT_TYPE_LINE:
			if (!int32x2IsDefined (ctx->curveBegin)) ctx->curveBegin = int32x2 (e.line.x[0], e.line.y[0]);
			ctx->curveEnd = int32x2 (e.line.x[1], e.line.y[1]);

			return fifoPrintString (ctx->output,"\t{\t.x={ ")
			&& btB2cPrintValue (ctx,e.line.x[0])
			&& fifoPrintString (ctx->output,", ")
			&& btB2cPrintValue (ctx,e.line.x[1])
			&& fifoPrintString (ctx->output," },\n\t\t.y={ ")
			&& btB2cPrintValue (ctx,e.line.y[0])
			&& fifoPrintString (ctx->output,", ")
			&& btB2cPrintValue (ctx,e.line.y[1])
			&& fifoPrintString (ctx->output,"},\n\t\t.speed0=")
			&& btB2cPrintValue (ctx,e.v0)
			&& fifoPrintString (ctx->output,", .speed1=")
			&& btB2cPrintValue (ctx,e.v0)
			&& fifoPrintString (ctx->output,", .command=BEZIER_COMMAND_LINE\n\t},\n");
		break;

		case BT_TYPE_BEZIER2: {
			if (!int32x2IsDefined (ctx->curveBegin)) ctx->curveBegin = int32x2 (e.bezier2.x[0], e.bezier2.y[0]);
			ctx->curveEnd = int32x2 (e.bezier2.x[2], e.bezier2.y[2]);

			return fifoPrintString (ctx->output,"\t{\t.x={ ")
			&& btB2cPrintValue (ctx,e.bezier2.x[0])
			&& fifoPrintString (ctx->output,", ")
			&& btB2cPrintValue (ctx,e.bezier2.x[1])
			&& fifoPrintString (ctx->output,", ")
			&& btB2cPrintValue (ctx,e.bezier2.x[2])
			&& fifoPrintString (ctx->output," },\n\t\t.y={ ")
			&& btB2cPrintValue (ctx,e.bezier2.y[0])
			&& fifoPrintString (ctx->output,", ")
			&& btB2cPrintValue (ctx,e.bezier2.y[1])
			&& fifoPrintString (ctx->output,", ")
			&& btB2cPrintValue (ctx,e.bezier2.y[2])
			&& fifoPrintString (ctx->output,"},\n\t\t.speed0=")
			&& btB2cPrintValue (ctx,e.v0)
			&& fifoPrintString (ctx->output,", .speed1=")
			&& btB2cPrintValue (ctx,e.v0)
			&& fifoPrintString (ctx->output,", .command=BEZIER_COMMAND_CURVE2\n\t},\n");
		} break;

		case BT_TYPE_LABEL:
			fifoPrintString (ctx->output,"\t// label ");
			fifoPrintString (ctx->output,e.label.text);
			return fifoPrintLf (ctx->output);
		break;

		case BT_TYPE_COMMENT:
			fifoPrintString (ctx->output,"\t// comment ");
			fifoPrintString (ctx->output,e.comment.text);
			return fifoPrintLf (ctx->output);
		break;

		case BT_TYPE_INLINED:
			return e.inlined.format!=OUTPUT_FORMAT_B2C
			|| fifoPrintString (ctx->output,e.inlined.text)
			&& fifoPrintLf (ctx->output);
		break;

		default: return false; 	// unknown element type
			// fprintf (stderr,"Invalid BT type.\n");
	}
	return false;
}

/** Writes the file's footer.
 * @param context the accumulated side effects of the previous elements.
 */
bool btB2cDone (BtB2cContext *ctx) {
	if (ctx->enumBeginEnd) return
		true
		&& fifoPrintString (ctx->output,"};\n")		// end of array
		&& fifoPrintString (ctx->output,"enum ")
		&& fifoPrintString (ctx->output,ctx->name)
		&& fifoPrintString (ctx->output,"Symbols {\n\t")
		&& fifoPrintString (ctx->output,ctx->name)
		&& fifoPrintString (ctx->output,"BeginX = ")
		&& btB2cPrintValue (ctx,ctx->curveBegin.x)
		&& fifoPrintString (ctx->output,",\n\t")
		&& fifoPrintString (ctx->output,ctx->name)
		&& fifoPrintString (ctx->output,"BeginY = ")
		&& btB2cPrintValue (ctx,ctx->curveBegin.y)
		&& fifoPrintString (ctx->output,",\n\t")
		&& fifoPrintString (ctx->output,ctx->name)
		&& fifoPrintString (ctx->output,"EndX = ")
		&& btB2cPrintValue (ctx,ctx->curveEnd.x)
		&& fifoPrintString (ctx->output,",\n\t")
		&& fifoPrintString (ctx->output,ctx->name)
		&& fifoPrintString (ctx->output,"EndY = ")
		&& btB2cPrintValue (ctx,ctx->curveEnd.y)
		&& fifoPrintString (ctx->output,",\n")
		&& fifoPrintString (ctx->output,"};\n");
	else return fifoPrintString (ctx->output,"};\n");	// end of array

}

////////////////////////////////////////////////////////////////////////////////////////////////////
// universal output

bool btSymbolsPrintValue (const BtOutputContextSimple *ctx, Int32 value_e20) {
	if (ctx->hex) return fifoPrintString (ctx->output,"0x") && fifoPrintHex (ctx->output, value_e20, 8,8);
	else return fifoPrintInt32Easy_e20 (ctx->output, value_e20,7);
}

bool btSymbolsPrintPoint (const BtOutputContextSimple *ctx, Int32x2 value_e20) {
	return	btSymbolsPrintValue (ctx,value_e20.x)
		&& fifoPrintSpace (ctx->output)
		&& btSymbolsPrintValue (ctx,value_e20.y);
}

bool btOutputSymbols (const BtOutputContextSimple *c, BtList *btList) {
// output: start, end, labels
 	Fifo *f = c->output;
	bool success = true, start = true;
	Int32x2 end = int32x2Undefined();
	const char *label = 0;
	Int32x2 minPoint = int32x2Undefined();
	Int32x2 maxPoint = int32x2Undefined();
	while (success && btListCanRead (btList)) {
		const BtElement e = btListRead (btList);
		if (e.type==BT_TYPE_LABEL) {
			label = e.label.text;
		}
		else if (btElementIsPath (e)) {
			const Int32x2 pointBegin = btElementPointBegin (e);


			if (start) {
				success = success && fifoPrintString (f,"start = ") && btSymbolsPrintPoint (c,pointBegin) && fifoPrintLf (f);
				start = false;
			}
			if (label!=0) {
				success = success
				&& fifoPrintString (f,label)
				&& fifoPrintString (f," = ") && btSymbolsPrintPoint (c,pointBegin) && fifoPrintLf (f);
				label = 0;
			}

			end = btElementPointEnd (e);

			// bounding box, etc.
			if (!int32x2IsDefined (minPoint)) {
				minPoint = pointBegin;
				maxPoint = pointBegin;
			}
			minPoint.x = int32Min (minPoint.x, pointBegin.x);
			minPoint.y = int32Min (minPoint.y, pointBegin.y);
			minPoint.x = int32Min (minPoint.x, end.x);
			minPoint.y = int32Min (minPoint.y, end.y);
			maxPoint.x = int32Max (maxPoint.x, pointBegin.x);
			maxPoint.y = int32Max (maxPoint.y, pointBegin.y);
			maxPoint.x = int32Max (maxPoint.x, end.x);
			maxPoint.y = int32Max (maxPoint.y, end.y);
		}
	}

	if (int32x2IsDefined (end)) success = success
		&& fifoPrintString (f,"end = ") && btSymbolsPrintPoint (c,end) && fifoPrintLf (f);
	if (int32x2IsDefined (minPoint)) success = success
		&& fifoPrintString (f,"connectionPointMin = ") && btSymbolsPrintPoint (c,minPoint)
		&& fifoPrintString (f,"\nconnectionPointMax = ") && btSymbolsPrintPoint (c,maxPoint)
		&& fifoPrintLf (f);

	return success;
}

bool btOutput (BtOutputContext *context, BtList *btList, int outputFormat) {
	bool success = true;
	switch(outputFormat) {
		case OUTPUT_FORMAT_BT: {
			success = success && btBezierToolBegin (&context->bt);
			while (success && btListCanRead (btList)) {
				success = success && btBezierToolElement (&context->bt, btListRead (btList));
			}
			success = success && btBezierToolDone (&context->bt);
		}
		break;
		case OUTPUT_FORMAT_SBT: {
			while (success && btListCanRead (btList)) {
				success = success && btStatelessBezierToolElement (&context->sbt, btListRead (btList));
			}
		}
		break;
		case OUTPUT_FORMAT_PS_RAYS:
		case OUTPUT_FORMAT_PS: {
			// convert to Postscript

			success = success && btPostscriptBegin (&context->ps);
			while (success && btListCanRead (btList)) {
				success = success && btPostscriptElement (&context->ps, btListRead (btList));
			}
			success = success && btPostscriptDone (&context->ps);
		}
		break;
		case OUTPUT_FORMAT_GCODE: {

			success = btGcodeBegin (&context->gcode);
			while (success && btListCanRead (btList)) {
				success = success && btGcodeElement (&context->gcode, btListRead (btList));
			}
			success = success && btGcodeDone (&context->gcode);
		}
		break;
		case OUTPUT_FORMAT_B2C: {

			success = btB2cBegin (&context->b2c);
			while (success && btListCanRead (btList)) {
				success = success && btB2cElement (&context->b2c, btListRead (btList));
			}
			success = success && btB2cDone (&context->b2c);
		}
		break;
		default: ;
	}
	return success;
}

bool btOutputSimple (const BtOutputContextSimple *ctx, BtList *btList, int outputFormat) {
	BtOutputContext oc = {
		.bt = { .output = ctx->output, .hex = ctx->hex, .noComments = ctx->noComments },
		.sbt = { .output = ctx->output, .hex = ctx->hex, .noComments = ctx->noComments },
		.ps = { .output = ctx->output, .format=outputFormat, .literalLabels = ctx->literalLabels, .noHeaders = ctx->noHeaders,
			.noFooters = ctx->noFooters, .coordinate=ctx->coordinate, .origin = ctx->origin,
			.pageOrigin = ctx->pageOrigin, .zoomOrigin=ctx->zoomOrigin, .pageZoom = ctx->pageZoom,
		},
		.gcode ={ .output = ctx->output, .literalLabels = ctx->literalLabels, .noHeaders = ctx->noHeaders, .axes = { ctx->axes[0], ctx->axes[1] }, },
		.b2c = { .output = ctx->output, .hex = ctx->hex, .name = ctx->name, .enumBeginEnd = ctx->optionalOutput, .noHeaders = ctx->noHeaders, },
	};
	// special handling for symbols output
	return outputFormat==OUTPUT_FORMAT_SYMBOLS ? btOutputSymbols (ctx,btList) : btOutput (&oc,btList,outputFormat);
}

