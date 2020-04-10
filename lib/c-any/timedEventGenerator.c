#include <timedEventGenerator.h>

void timedEventGeneratorStart(TimedEventGenerator *teg, int tNow) {
	teg->tOffset = tNow;
	teg->state = 0;
	teg->event = teg->samples[teg->n-1].event;
	teg->cycles = 0;
	teg->run = true;
}

void timedEventGeneratorPause(TimedEventGenerator *teg) {
	teg->run = false;
}

void timedEventGeneratorContinue(TimedEventGenerator *teg, int tNow) {
	teg->tOffset = tNow;
	teg->state = 0;
	teg->run = true;
}

bool timedEventGeneratorUpdate(TimedEventGenerator *teg, int t) {
	const int dT = t - teg->tOffset;
	const TimedEvent *te = &teg->samples[teg->state];

	if (teg->state==-1) return false;	// stopped before
	else if (dT>=te->t) {	// next transition
		const bool change = te->event != teg->event;
		teg->event = te->event;
		if (teg->state==teg->n-1) {	// last match = cycle time
			teg->state = teg->run ? 0 : -1;
			teg->tOffset += te->t;
			teg->cycles ++;
		}
		else teg->state++;
		return change;
	}
	else return false;
}


