//HEADER
#include <smallButton.h>

//SLICE
bool smallButtonSample (SmallButton *btn, Uint8 tDead, Uint8 tNow, bool value) {
	// check dead-time first...
	if ((btn->state & SMALLBUTTON_STATE_RELEASED) && (Int8)(tNow - btn->tAction - tDead) >=0 ) btn->state = SMALLBUTTON_STATE_ENGAGED;

	if (value) {
		if (btn->state==SMALLBUTTON_STATE_ENGAGED) {
			btn->state = SMALLBUTTON_STATE_PRESSED | SMALLBUTTON_FLAG_PRESSED;
			btn->tAction = tNow;
			return true;
		}
		else return false;	// auto-repeat is NOT checked here - too expensive for simple implementations
	}
	else {
		if (btn->state & SMALLBUTTON_STATE_PRESSED) {	// any 'pressed' type 
			btn->state = SMALLBUTTON_STATE_RELEASED | SMALLBUTTON_FLAG_RELEASED;
			btn->tAction = tNow;
			return true;
		}
		else return false;
	}
}

//SLICE
bool smallButtonSampleAutoRepeat (SmallButton *btn, Uint8 tDelay, Uint8 tRepeat, Uint8 tNow) {
	if (btn->state & SMALLBUTTON_STATE_PRESSED) {	// still pressed (smallButtonSample's check)
		if ((btn->state & SMALLBUTTON_STATE_REPEATED)==SMALLBUTTON_STATE_REPEATED) {	// auto-repeat active
			if ((Int8)(tNow - btn->tAction - tRepeat)>=0) {	// last repeat is tRepeat ago
				btn->state |= SMALLBUTTON_FLAG_REPEATED;
				btn->tAction += tRepeat;
				return true;
			}
			else return false;
		}
		else {	// first repeat, possibly...
			if ((Int8)(tNow - btn->tAction - tDelay)>=0) {		// yes, first press is tDelay ago
				btn->state |= SMALLBUTTON_STATE_REPEATED | SMALLBUTTON_FLAG_HELD | SMALLBUTTON_FLAG_REPEATED;
				btn->tAction += tDelay;
				return true;
			}
			else return false;
		}
	}
	else return false;
}

//SLICE

/** Performs full-featured button feeding and returns an action, if available. This is an expensive function in terms of
 * code size.
 * @param btn the SmallButton object (and state).
 * @param allTimings deadTime, delay and auto-repeat time put into one 32bit integer in this order
 * @param tNow the current time
 * @param value the sample value of the button.
 */
ButtonAction smallButtonActionAutoRepeat (SmallButton *btn, Uint32 allTimings, Uint8 tNow, bool value) {
	const Uint8
		tDead = allTimings & 0xFF,
		tDelay = allTimings>>8 & 0xFF,
		tRepeat = allTimings>>16 & 0xFF;

	smallButtonSample (btn,tDead,tNow,value);
	smallButtonSampleAutoRepeat (btn,tDelay,tRepeat,tNow);

	if (smallButtonPressedEdge (btn))		return BUTTON_ACTION_PRESSED;
	else if (smallButtonReleasedEdge (btn)) 	return BUTTON_ACTION_RELEASED;
	else if (smallButtonHeldEdge (btn))		return BUTTON_ACTION_HELD;
	else if (smallButtonRepeatedEdge (btn))		return BUTTON_ACTION_REPEATED;
	else						return BUTTON_ACTION_IDLE;
}
