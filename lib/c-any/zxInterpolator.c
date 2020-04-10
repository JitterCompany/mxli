#include <zxInterpolator.h>
#include <int64Math.h>

bool zxInterpolatorFeed(ZxInterpolator *zxi, int t1, int f1) {
	const bool zx = zxi->f0 >= 0 ?
		(f1 < 0 ? true : false) : (f1 >= 0 ? true : false);

	if (zx) {
		zxi->zx = int64Div(f1*zxi->t0 - zxi->f0*t1, f1 - zxi->f0);
	}

	zxi->t0 = t1;
	zxi->f0 = f1;

	return zx;
}

int zxInterpolatorGetZx(const ZxInterpolator *zxi) {
	return zxi->zx;
}
