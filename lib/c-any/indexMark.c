#include <indexMark.h>
#include <macros.h>

void indexMarkUpdate(IndexMark* i, int tUs) {
	if (i->cycleUs>0) {	// running
		i->cycleUs = tUs - i->zUs;
		// average16 = average16*(1-1/16) + cycleUs
		const int subtract = i->sumCycleUs >> i->e;
		i->sumCycleUs += i->cycleUs - subtract;
	}
	else if (i->cycleUs==0)	i->cycleUs = -1;		// first value
	else {	// second value; cycleUs < 0
		i->cycleUs = tUs - i->zUs;
		i->sumCycleUs = i->cycleUs << i->e;
	}
	i->zUs = tUs;
}

bool indexMarkIsValid(const IndexMark *i) {
	return i->cycleUs > 0;
}

int indexMarkCycleUs(const IndexMark* i) {
	return i->sumCycleUs >> i->e;
}

bool indexMarkIsRotating(const IndexMark *i, int tUs, int tCycleMaxUs) {
	return tUs - i->zUs < tCycleMaxUs;
}

bool indexMarkSpeedIsClose(const IndexMark *reference, const IndexMark *index, int fraction) {
	if (indexMarkIsValid(reference) && indexMarkIsValid(index)) {
		const int deltaUs = reference->cycleUs - index->cycleUs;
		return ABS(deltaUs)*fraction <= reference->cycleUs;
	}
	else return false;
}

bool indexMarkAvgSpeedIsClose(const IndexMark *reference, const IndexMark *index, int fraction) {
	if (indexMarkIsValid(reference) && indexMarkIsValid(index)) {
		const int deltaUs = reference->sumCycleUs - index->sumCycleUs;
		return ABS(deltaUs)*fraction <= reference->sumCycleUs;
	}
	else return false;
}

