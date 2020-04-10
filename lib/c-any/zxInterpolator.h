#ifndef zxInterpolator_h
#define zxInterpolator_h

#include <stdbool.h>

/** Zero crossing interpolator.
 * Provided we have a function linear in time that has a zero crossing at unknown time tz but we only have samples
 * at certain points in time, then this module calculates the time of the zero crossing by linear interpolation.
 */
struct ZxInterpolator {
	int	t0;		///< latest sampling time
	int	f0;		///< latest function sample
	int	zx;		///< latest zero crossing
};
typedef struct ZxInterpolator ZxInterpolator;

/** Feeds the next sample into the interpolator.
 * @param zxi the interpolator object.
 * @param t the sampling time
 * @param fSample the function sample value
 * @return true, if a zero crossing happened.
 */
bool zxInterpolatorFeed(ZxInterpolator *zxi, int t, int fSample);

/** Calculates the latest zero crossing time.
 * @param zxi the interpolator object.
 * @return the exact time of the zero crossing.
 */
int zxInterpolatorGetZx(const ZxInterpolator *zxi);

#endif
