#include <c-linux/bezierToSource.h>
#include <stdio.h>
#include <int32Math.h>

void fprintfPoint (FILE *f, Int32x2 p) {
	fprintf (f,"{ 0x%08x, 0x%08x },",p.x,p.y);
}

void fprintfCommentPoint (FILE *f, int e, Int32x2 p) {
	fprintf (f,"\t//(%f,%f),",p.x*1.0/(1<<e),p.y*1.0/(1<<e));
}

bool int32x2CurvePrintAsCFile_en (int e, const char* fn, const char* curveName, const Int32x2Curve *curve) {
	double one = 1.0f/(1<<e);
	FILE *f = fopen (fn,"w");
	Int32x2Fifo points = *curve->segments;
	if (f!=0) {
		fprintf(f, "// curve area center of gravity is (0,0)\n");
		fprintf(f, "enum {\t%sOx=0x%08x, %sOy=0x%08x,\t// Curve origin at (%f,%f)\n",
			curveName,curve->x0.x,curveName,curve->x0.y,
			curve->x0.x * one, curve->x0.y * one
			);
		const Int32x2 z = int32x2CurveEnd (curve);
		fprintf(f, "\t%sEx=0x%08x, %sEy=0x%08x }; \t// Curve end at (%f,%f)\n\n",
			curveName,z.x,curveName,z.y,
			z.x * one, z.y * one
			);
		fprintf(f, "const Int32x2 %s[] = {\n",curveName);
		fprintf (f,"\t// start point omitted here!\n");

		for (int segno=0; int32x2FifoCanRead (&points)>=2; segno++) {
			const Int32x2 x1 = int32x2FifoRead (&points);
			const Int32x2 x2 = int32x2FifoRead (&points);
			fprintf (f,"\t");
			fprintfPoint (f,x1); fprintfPoint (f,x2);
			fprintfCommentPoint (f,e,x1); fprintfCommentPoint (f,e,x2);
			fprintf (f,"\t: Bezier2 segment [%d]\n",segno);
		}
		fprintf (f,"};\n");
		return true;
	}
	else return false;
}

bool int32x2CurvePrintAsGnuplot_en (int e, const char* fn, const Int32x2Curve *curve) {
	FILE *f = fopen (fn,"w");
	const double scale = 1.0/(1<<e);

	fprintf (f,"%d %f\t%f\n",0,curve->x0.x*scale,curve->x0.y*scale);
	Int32x2Fifo points = *curve->segments;
	if (f!=0) {
		for (int i=1; int32x2FifoCanRead (&points)!=0; i++) {
			const Int32x2 x = int32x2FifoRead (&points);
			fprintf (f,"%d %f\t%f\n",i,x.x*scale,x.y*scale);
		}
		return true;
	}
	else return false;
}

bool int32x2CurvePrintAsCFileBezierCommands_en (int e, const char* fn, const char* curveName, const Int32x2Curve *curve, Int32 speed_en) {
	FILE *f = fopen (fn,"w");
	Int32x2Fifo points = *curve->segments;
	if (f!=0) {
		fprintf(f, "const Bezier2Command %s[] = {\n\t",curveName);
		Int32x2 x0 = curve->x0;
		for (int segno=0; int32x2FifoCanRead (&points)>=2; segno++) {
			const Int32x2 x1 = int32x2FifoRead (&points);
			const Int32x2 x2 = int32x2FifoRead (&points);
			fprintf (f,"\t{\t// Bezier2 segment [%d]\n",segno);
			fprintf (f,"\t\t{ 0x%08x, 0x%08x, 0x%08x },\n", x0.x, x1.x, x2.x);
			fprintf (f,"\t\t{ 0x%08x, 0x%08x, 0x%08x },\n", x0.y, x1.y, x2.y);
			fprintf (f,"\t\t.speed0=0x%08x, .speed1=0x%08x, BEZIER_COMMAND_CURVE\n",speed_en,speed_en);
			fprintf (f,"\t},\n");
			x0 = x2;
		}
		fprintf (f,"};\n");
		return true;
	}
	else return false;
}

bool int32x2CurvePrintAsCFileBezierCommandsPolar_en (int e, const char* fn, const char* curveName, const Int32x2Curve *curve, Int32 speed_en, int id) {
	enum {
		MAX_SLOW_DOWN = 512,
		MAX_SPEED_UP = 64,
	};

	double one = 1.0f/(1<<e);
	FILE *f = fopen (fn,"w");
	Int32x2Fifo points = *curve->segments;
	if (f!=0) {
		Int32x2 x0 = curve->x0;
		fprintf(f, "enum {\t%sOx=0x%08x, %sOy=0x%08x,\t// Curve origin at (%f,%f)\n",
			curveName,x0.x,curveName,x0.y,
			x0.x * one, x0.y * one
			);
		const Int32x2 z = int32x2CurveEnd (curve);
		fprintf(f, "\t%sEx=0x%08x, %sEy=0x%08x }; \t// Curve end at (%f,%f)\n\n",
			curveName,z.x,curveName,z.y,
			z.x * one, z.y * one
			);
		fprintf(f, "const Bezier2Command %s[] = {\n",curveName);
		for (int segno=0; int32x2FifoCanRead (&points)>=2; segno++) {
			const Int32x2 x1 = int32x2FifoRead (&points);
			const Int32x2 x2 = int32x2FifoRead (&points);
			const Int32x2 v0Can = int32x2Scale_en (e,int32x2Sub (x1,x0), 2<<e);
			const Int32x2 v2Can = int32x2Scale_en (e,int32x2Sub (x2,x1), 2<<e);
			const Int32x2 v0Phys = {
				v0Can.x,						// d/dt r as usual
				((Int64)v0Can.y*(PIx2_E29/2)>>28) * x0.x >> e		// 2*PI*(d/dt y) * r
			};
			const Int32x2 v2Phys = {
				v2Can.x,						// d/dt r as usual
				((Int64)v2Can.y*(PIx2_E29/2)>>28) * x2.x >> e		// 2*PI*(d/dt y) * r
			};

			const Uint32 v0CanAbs = int32x2Abs (v0Can);
			const Uint32 v2CanAbs = int32x2Abs (v2Can);
			const Uint32 v0PhysAbs = int32x2Abs (v0Phys);
			const Uint32 v2PhysAbs = int32x2Abs (v2Phys);
			fprintf (stderr,"seg[%d]: v0CanAbs=%f\n",segno,int32x2Abs(v0Can)*1.0/(1<<e));
			fprintf (stderr,"seg[%d]: v2CanAbs=%f\n",segno,int32x2Abs(v2Can)*1.0/(1<<e));
			fprintf (stderr,"seg[%d]: v0PhysAbs=%f\n",segno,v0PhysAbs*1.0/(1<<e));
			fprintf (stderr,"seg[%d]: v2PhysAbs=%f\n",segno,v2PhysAbs*1.0/(1<<e));
			if (v0PhysAbs==0) fprintf (stderr,"WARNING: v0Abs==0 in segment [%d]\n",segno);
			if (v2PhysAbs==0) fprintf (stderr,"WARNING: v2Abs==0 in segment [%d]\n",segno);

			Int32 speed0_en = uint64Div ((Uint64)speed_en*v0CanAbs, v0PhysAbs);
			Int32 speed2_en = uint64Div ((Uint64)speed_en*v2CanAbs, v2PhysAbs);

			bool lim0 = false;
			bool lim2 = false;
			if (speed0_en < speed_en/MAX_SLOW_DOWN) {
				fprintf (stderr,"WARNING: speed0_en too low in segment [%d]\n",segno);
				speed0_en = speed_en / MAX_SLOW_DOWN;
				lim0 = true;
			}
			if (speed0_en > (Int64) speed_en*MAX_SPEED_UP) {
				fprintf (stderr,"WARNING: speed0_en too high in segment [%d]\n",segno);
				speed0_en = speed_en * MAX_SPEED_UP;
				lim0 = true;
			}

			if (speed2_en < speed_en/MAX_SLOW_DOWN) {
				fprintf (stderr,"WARNING: speed2_en too low in segment [%d]\n",segno);
				speed2_en = speed_en / MAX_SLOW_DOWN;
				lim2 = true;
			}
			if (speed2_en > (Int64) speed_en*MAX_SPEED_UP) {
				fprintf (stderr,"WARNING: speed2_en too high in segment [%d]\n",segno);
				speed2_en = speed_en * MAX_SPEED_UP;
				lim2 = true;
			}
			fprintf (stderr,"seg[%d]: speed0=%f\n",segno,speed0_en*1.0/(1<<e));
			fprintf (stderr,"seg[%d]: speed2=%f\n",segno,speed2_en*1.0/(1<<e));

			fprintf (f,"\t{\t// Bezier2 segment [%d], id=%d\n",segno,id+segno);
			fprintf (f,"\t\t{ 0x%08x, 0x%08x, 0x%08x },\n", x0.x, x1.x, x2.x);
			fprintf (f,"\t\t{ 0x%08x, 0x%08x, 0x%08x },\n", x0.y, x1.y, x2.y);
			fprintf (f,"\t\t.speed0=0x%08x, .speed1=0x%08x,%s",speed0_en,speed2_en, ((lim0||lim2) ? "\t(fixed)" : "") );
			fprintf (f,"\t// %f, %f\n",speed0_en*1.0/(1<<e), speed2_en*1.0/(1<<e));
			fprintf (f,"\t\tBEZIER_COMMAND_CURVE,.id=%d\n",id+segno);
			fprintf (f,"\t},\n");
			x0 = x2;
		}
		fprintf (f,"};\n");
		return true;
	}
	else return false;
}
