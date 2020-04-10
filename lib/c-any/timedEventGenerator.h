#ifndef __timedEventGenerator_h
#define __timedEventGenerator_h

#include <stdbool.h>

typedef struct {
	int		t;	///< time
	int		event;	///< output/event
} TimedEvent;

/** Periodic event generator.
 *
 */
typedef struct {
	const TimedEvent*	samples;
	int			n;
	int			state;
	int			tOffset;
	int			event;		///< most recent event
	int			cycles;		///< counts of full cycles
	volatile bool		run;
} TimedEventGenerator;

/** Starts the event timer.
 * @param teg the event timer object
 * @param tNow the current time or a time in future for cycle start.
 */
void timedEventGeneratorStart(TimedEventGenerator *teg, int tNow);

/** Pauses the event timer. The current cycle is finished, however.
 * @param teg the event timer object
 */
void timedEventGeneratorPause(TimedEventGenerator *teg);

/** Continues the event timer. The time to start continuing must be provided.
 * @param teg the event timer object
 * @param tNow the current time or a time in future for cycle start.
 */
void timedEventGeneratorContinue(TimedEventGenerator *teg, int tNow);

/** Performs state transitions of the event timer.
 * @param teg the event timer object
 * @param t the current time.
 * @return true, if the event status (output) changed, false otherwise.
 */
bool timedEventGeneratorUpdate(TimedEventGenerator *teg, int t);

/** Extracts the latest event value. This value can also be seen as the output level of the event timer.
 * @param teg the event timer object
 * @return the latest event value.
 */
inline static int timedEventGeneratorValue(const TimedEventGenerator *teg) {
	return teg->event;
}

#endif

