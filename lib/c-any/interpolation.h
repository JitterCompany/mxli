#ifndef __interpolation_h
#define __interpolation_h

/** @file
 * @brief Various integer-based interpolation utilities.
 *
 * Movement models constant acceleration/deceleration. This is usefull for motor movements with soft characteristics.
 *
 * Another field of interpolation is the approximation of functions by samples. This is called sampled functions here
 * and the the letters 'Sf' stand for 'sampled function' in the names used.
 *
 * Int32Sf is a function sampled using (x,y)
 * samples. This gives most freedom in defining the function, but has lowest performance O(n), if n is the number of
 * samples. Moreover, a decision has to be made whether to use the slow 64-bit division (on 32-bit architectures) to
 * be able to use the full integer range for x and y or to use 32-bit division only, which restricts the ranges to
 * x*y <= 32bit.
 *
 * Int32Fsf is a faster version of a sampled functions. It uses constant sample distances (x) in order to improve
 * complexity to O(1). As with Int32Sf there's a trade-off between speed of calculation and range of values.
 *
 * Int32Usf is the 'ultra-fast sampled function'. It uses sample distances (x) of a constant value 2^w. This not only
 * allows for a faster table look-up, but also has the advantage of both high speed and full range of values for both
 * x and y. Complexity is O(1).
 * As an example: on Cortex-M3 (LPC1766) Int32Usf outperforms Int32Fsf for full range values by a factor of 4.
 */
#include <integers.h>
#include <stdbool.h>
#include <fifo.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Fractional counter
//

/** A fractional divider/multiplier for a wide range of ratios.
 * Roughly speaking, every call to fractionalCounter 'increments' the counter by deltaX/deltaY. Only steps of at least
 * 1 are returned.
 */
typedef struct {
	int	deltaX;		///< divider
	int	deltaY;		///< multiplier
	int	z;		///< current value modulo integral parts
} FractionalCounter;

/** Initializes a fractional counter.
 * @param fc the fractional counter to initialize.
 * @param deltaX number of calls required for a deltaY step.
 * @param deltaY number of integral steps after deltaX calls.
 */
static inline void fractionalCounterInit(FractionalCounter *fc, int deltaX, int deltaY) {
	fc->deltaX = deltaX;
	fc->deltaY = deltaY;
	fc->z = deltaX>>1;	// z0 = deltaX/2, this is rounding up.
}

/** Advance one step.
 * @param fc the fractional counter.
 * @return the number of output steps to perform.
 */
static inline int fractionalCounter(FractionalCounter *fc) {
	fc->z += fc->deltaY;
	const int out = fc->z / fc->deltaX;
	fc->z -= out*fc->deltaX;
	return out;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Accelerated motion interpolation, incremental.
//

/** Movement limits. All units are with respect to one 'clock' of the interpolation loop. If this loop is running at
 * 20 000 cycles/s then the time unit is 50us. vMax = 2^20 / 20 000 is then one unit of movement / second.
 * a = 1/20 000 * 1/20 000 = approx. 2^-28 is then an acceleration of one unit of movement / second^2. This is
 * achieved by setting a_p to -28.
 */
typedef struct {
	Uint32	vMax_e20;	///< maximum absolute value of speed in units of 2^-32. Maybe better _e20?
	Int8	a_p;		///< acceleration value in exponential units: a = 2^-a_p, range -0..-31
	Uint32	tPause;		///< how much time to rest after stop - this prevent acceleration inversal at no time.
} MovementConfiguration;


/** Movement control structure.
 */
typedef struct {
	int			t1;	///< end of acceleration, start of linear interpolation
	int			t2;	///< start of deceleration
	int			t3;	///< end of deceleration/movement, start of pause
	int			t4;	///< end of pause
	int			s0;	///< start of movement
	int			s3;	///< end of movement

	Int8			dir;
	Int8			a_p;	///< exponent of a, 0 or negative

	int			t;	///< current clock time from starting point.
	int			v_e20;	///< current speed 1/clock / 2^20
	int			s;	///< current position, stored to avoid re-calculation
	FractionalCounter	linear;	///< incremental yet precise linear interpolation
	volatile bool		active;	///< movement is active - must not be changed.
} Movement;

/** Initializes the movement object. The Object is standing still. This function can be used to change the current
 * position of a movement object if it is not active at the moment of call.
 * @param m the movement object.
 * @param sCurrent the current position.
 */
void movementInit(Movement *m, int sCurrent);

/** Calculates the parameters for a movement from the current position to the target position.
 * The movement consists of a constant acceleration up to a maximum speed, a constant speed period and finally
 * a constant deceleration down to speed 0 while exectly reaching the target position at speed 0.
 * @param m the movement control structure to prepare.
 * @param mc the movement constraints acceleration and maximum speed.
 * @param sTarget the target of the movement, speed will be zero at this position again.
 */
void movementStart(Movement *m, MovementConfiguration const *mc, int sTarget);

/** Forced movement stop. The result is a leap ahead in time to the final destination, just before the pause. 
 * The movement still takes the time for the final pause. The pause will not be restarted by a following call.
 * This function violates the principles of Movement. In a real
 * world, however we often need an emergency stop.
 * @param m the movement object.
 */
void movementExit(Movement *m);

/** Forced movement stop. Speed is set to zero, the current position preserved. 
 * The movement still takes the time for the final pause. The pause will not be restarted by a following call.
 * This function violates the principles of Movement. In a real
 * world, however we often need an emergency stop.
 * @param m the movement object.
 */
void movementFreeze(Movement *m);

/** Checks, if the final destination is reached. Time is considered, not movement, because quantization would lead
 * to premature abort. Also, the pause time is respected
 */
static inline int movementDone(const Movement *m) {
	return ! m->active;
}

/** Gets the current position without moving.
 * @param m the (initialized!) movement control structure.
 * @return the current position.
 */
static inline int movementPosition(const Movement *m) {
	return m->s;
}

/** Gets the position of the point where the movement starts/started.
 * @param m the (initialized!) movement control structure.
 */
static inline int movementOrigin(const Movement *m) {
	return m->s0;
}

/** Gets the position of the point where the movement ends/ended.
 * @param m the (initialized!) movement control structure.
 */
static inline int movementDestination(const Movement *m) {
	return m->s3;
}

/** Gets the current clock counter which is the number of calls to the function movementClock().
 * @param m the (initialized!) movement control structure.
 * @return a value of 0 after initialization and a positive value afterwards.
 */
static inline int movementClocks(const Movement *m) {
	return m->t;
}

/** Gets the current speed.
 * @param m the (initialized!) movement control structure.
 * @return the current speed in units/clock / 2^20. This is a signed value with negative values meaning backward
 *   movement.
 */
static inline int movementSpeed_e20(const Movement *m) {
	return m->v_e20;
}

/** Moves ahead in time one clock. The new position is calculated and returned.
 * @param m the movement control structure.
 * @return the new position after the step.
 */
int movementClock(Movement *m);

////////////////////////////////////////////////////////////////////////////////////////////////////
// sampled functions

typedef struct {
	Int32	x;
	Int32	y;
} Int32Point;

/** Curve defined by a finite number of samples. The x-values have to be in ascending order.
 */
typedef struct {
	const Int32Point*	points;
	int			elements;		///< number of points 
} Int32Sf;

/** Gets a sample point by index.
 */
static inline Int32Point int32SfPoint(const Int32Sf *sf, int i) {
	return sf->points[i];
}

/** Gets the first sampled point. This is - by convention - the leftmost point.
 */
static inline Int32Point int32SfLeft(const Int32Sf *sf) {
	return sf->points[0];
}

/** Gets the last sampled point. This is - by convention - the rightmost point.
 */
static inline Int32Point int32SfRight(const Int32Sf *sf) {
	return sf->points[sf->elements -1];
}

/** Performs linear interpolation between the sample points.
 * @param sf the samples defining the function
 * @param x the point at which the function is evaluated
 * @return the function return value derived by linear interpolation. Outside the samples' region no extrapolation
 *   is performed - the function sticks to the first or last sample value.
 */
Int32 int32SfLinearInterpolate(const Int32Sf *sf, Int32 x);

/** Fast linear interpolated function. Equidistant samples of a curve.
 */
typedef struct {
	Int32*		samples;
	int		elements;		///< number of samples
	Int32		sampleDistance;
	Int32		origin;
} Int32Fsf;

/** Gets the first sampled point. This is - by convention - the leftmost point.
 */
static inline Int32Point int32FsfLeft(const Int32Fsf *sf) {
	const Int32Point p = {
		.x = sf->origin,
		.y = sf->samples[0]
	};
	return p;
}

/** Gets the last sampled point. This is - by convention - the rightmost point.
 */
static inline Int32Point int32FsfRight(const Int32Fsf *sf) {
	const Int32Point p = {
		.x = sf->origin + (sf->elements-1)*sf->sampleDistance,
		.y = sf->samples[sf->elements-1]
	};
	return p;
}

/** Initializes the samples from a (probably slower) sampled function.
 */
bool int32FsfFromInt32Sf(Int32Fsf *fsf, Int32* samples, int bufferSize, const Int32Sf *sf);

/** Evaluates the function at a given point. 32-bit math used, only.
 */
Int32 int32FsfLinearInterpolateFast(const Int32Fsf *fsf, Int32 x);

/** Evaluates the function at a given point. 64-bit math used.
 */
Int32 int32FsfLinearInterpolate(const Int32Fsf *fsf, Int32 x);


/** Ultra fast linear interpolated function. Equidistanc samples, distance = 2^x.
 * This interpolation can supply full integer range in both x and y while still avoiding the time-consuming 64-bit
 * integer division. The integer division can be replaced by a bit-shift here.
 */
typedef struct {
	Int32*		samples;	///< value samples
	int		elements;	///< size of the samples array
	Int32		shift;		///< exponential divider
	Int32		origin;		///< offset
} Int32Usf;

/** Gets the first sampled point. This is - by convention - the leftmost point.
 */
static inline Int32Point int32UsfLeft(const Int32Usf *sf) {
	const Int32Point p = {
		.x = sf->origin,
		.y = sf->samples[0]
	};
	return p;
}

/** Gets the last sampled point. This is - by convention - the rightmost point.
 */
static inline Int32Point int32UsfRight(const Int32Usf *sf) {
	const Int32Point p = {
		.x = sf->origin + (sf->elements-1)<<sf->shift,
		.y = sf->samples[sf->elements]
	};
	return p;
}

/** Calculates the left sample index to a 'x' value.
 */
static inline int int32UsfIndex(const Int32Usf *sf, Int32 x) {
	const int i = (x - sf->origin) >> sf->shift;
	if (i<0) return 0;
	else if (i>=sf->elements) return sf->elements-1;
	else return i;
}

/** Initializes the samples from a (probably slower) sampled function.
 * @return true, if the sampling frequency is high enough to reproduce the original function sufficiently well,
 *   false otherwise.
 */
bool int32UsfFromInt32Sf(Int32Usf *usf, Int32* samples, int bufferSize, const Int32Sf *sf);

/** Records a function by sampling.
 * @return true, if sampling was good, false if sampling was too coarse.
 */
bool int32UsfSampleInt32Sf(Int32Usf *usf, const Int32Sf *sf);

/** Uses 32-bit math.
 */
Int32 int32UsfLinearInterpolateFast(const Int32Usf *usf, Int32 x);

/** Uses 64-bit math.
 */
Int32 int32UsfLinearInterpolate(const Int32Usf *usf, Int32 x);

/** Prints samples of a USF.
 * @param fifo the output destination
 * @param usf the interpolation
 * @param n the number of samples to calculate.
 * @return true in case of succes, false if fifo overflown.
 */
bool fifoPrintInt32UsfSampled(Fifo *fifo, const Int32Usf *usf, int n);

/** Prints the key points of a USF.
 * @param fifo the output destination
 * @param usf the interpolation
 * @return true in case of succes, false if fifo overflown.
 */
bool fifoPrintInt32Usf(Fifo *fifo, const Int32Usf *usf);

#endif
