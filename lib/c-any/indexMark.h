#ifndef __indexMark_h
#define __indexMark_h

/** @file
 * @brief Model of an index mark of a rotating device, like a colour wheel.
 */

#include <stdbool.h>

//#warn "Index mark has changed."

typedef struct {
	int	zUs;		///< latest zero crossing
	int	cycleUs;	///< 0 for undefined IndexMark, -1 for zUs valid, else is undefined.
	int	sumCycleUs;	///< 16 value average, 134s maximum value
	int	e;		///< exponent, 4 for 16 values, 0 for NO averaging
} IndexMark;

/** Updates index mark.
 * @param i the index mark object.
 * @param tUs the time of the next index signal.
 */
void indexMarkUpdate(IndexMark* i, int tUs);

/** Checks, if the index mark's values are valid.
 * @param i the index mark object.
 * @return true, if all fields of the index mark object are valid.
 */
bool indexMarkIsValid(const IndexMark* i);

/** Checks, if the index mark still rotates.
 * @param i the index mark object.
 * @param tUs the current time.
 * @param tCycleMaxUs
 * @return true, if the index mark had a signal within tCycleMaxUs, false otherwise
 */
bool indexMarkIsRotating(const IndexMark *i, int tUs, int tCycleMaxUs);

/** Calculates the current cycle time.
 * @param i the index mark object.
 * @return the current cycle time in us or 0 if still unknown.
 */
int indexMarkCycleUs(const IndexMark *i);

/** Compares the frequencies of the index marks.
 * @param reference reference index mark, must be valid.
 * @param index second index mark, may be undefined.
 * @param fraction the fraction of cycle time, that is allowed as deviation.
 * @return true, if the cycle times of one index marks do not differ more than 1/fraction, false otherwise.
 */
bool indexMarkSpeedIsClose(const IndexMark *reference, const IndexMark *index, int fraction);

/** Compares the frequencies of the index marks.
 * @param reference reference index mark, must be valid.
 * @param index second index mark, may be undefined.
 * @param fraction the fraction of cycle time, that is allowed as deviation.
 * @return true, if the cycle times of one index marks do not differ more than 1/fraction, false otherwise.
 */
bool indexMarkAvgSpeedIsClose(const IndexMark *reference, const IndexMark *index, int fraction);

#endif
