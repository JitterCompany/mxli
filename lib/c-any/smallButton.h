/*
  smallButton.h 
  Copyright 2015,2016 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __smallButton_h
#define __smallButton_h

#include <integers.h>
#include <stdbool.h>
#include <button.h>

/** @file
 * @brief Abstract functionality for polled buttons using a small (2-byte RAM) footprint per button. 
 *
 * SmallButton provides the most basic functionality in a slim implementation. This is enough for many applications.
 * SmallButton responds immediately to a press but needs some time of constant release until it can be fired again.
 * This basic functionality can be extended to auto-repeat by calling an additional function.
 *
 * Care has to be taken on the times provided to the functions: only 8 bits are used, so a suitable timer is required.
 * A good example is usTimerUs() >> 13 which maps to 8ms / tick allowing for a range of at least 1s.
 * Button responds immediately to a press but needs some time of constant release until it can be fired again.
 * This implementation is spike sensitive - short pulses cause a button pressed event.
 *
 * Button press and button repeat are two totally different events and this proved useful in quite a few of my simple
 * user-interfaces already. That's the reason, why I introduce here the 'button held' event which is the one I'm after
 * most times I misuse a auto-repeat event for it. The button held event is fired only once per prolonged button press
 * and it can be used independently from auto-repeat events.
 * This Button implementation does not safe events for later query - a button press must be queried while the button
 * is held down otherwise it's gone. All other events behave similar. Fetch them quickly (you want a responsive 
 * user-interface, don't you ?) or lose them. For the same reason, smallButton does not need long time measurements -
 * one justification for its small RAM footprint.
 *
 * This button implementation is not thread safe !!!
 */

/** The order of the following constants are carefully chosen to simplify code below. Take care when changing!
 * They are chosen to be easily decoded by bit-testing.
 */
enum {
	// these 4 are for the query functions
	SMALLBUTTON_FLAG_PRESSED	=0x01,		///< a button press (not an auto-repeat)
	SMALLBUTTON_FLAG_HELD		=0x02,		///< auto-repeat start edge, used to detect the 'held' event.
	SMALLBUTTON_FLAG_REPEATED	=0x04,		///< auto-repeat start edge, used to detect the 'held' event.
	SMALLBUTTON_FLAG_RELEASED	=0x08,		///< a button release (not an auto-release)

	// these are for the state machine
	SMALLBUTTON_STATE_ENGAGED	=0,		///< smallButton has been released and dead time is over
	SMALLBUTTON_STATE_PRESSED	=0x10,
	SMALLBUTTON_STATE_REPEATED	=0x20|SMALLBUTTON_STATE_PRESSED,
	SMALLBUTTON_STATE_RELEASED	=0x40,
};

typedef struct {
	Uint8	tAction;	///< 10ms units (systickTimer?) recommended
	Uint8	state;
} SmallButton;

/** This module's recommendations for delay and repeats. Units are centi-seconds (cs).
 */
enum SmallButtonTimings {
	SMALLBUTTON_DEAD_CS	=10,		///< wait for switches to stabilize
	SMALLBUTTON_DELAY_CS	=25,		///< human-senseable delay
	SMALLBUTTON_REPEAT_CS	=10,		///< 6 presses/second auto-repeat
};

/** Checks, if smallButton is (still) down.
 * @param btn the SmallButton object (and state).
 * @return true, if button is down, false if button is up.
 */
inline static bool smallButtonIsDown (const SmallButton *btn) {
	return (btn->state & SMALLBUTTON_STATE_PRESSED) != 0;
}

/** Provides the next sample value of the smallButton. This is the minimal implementation.
 * @param btn the SmallButton object (and state).
 * @param tDead the dead-time that has to pass until button can be recognized as 'pressed' again.
 * @param tNow the current time in the range 0..255 .
 * @param value the button sample value with true meaning 'button down' and false meaning 'button up'.
 * @return true, if the smallButton state changed, false otherwise.
 */
bool smallButtonSample (SmallButton *btn, Uint8 tDead, Uint8 tNow, bool value);

/** Campanion to smallButtonSample, if auto-repeat and button-held are to be detected additionally.
 * Call this function after smallButtonSample(), if required.
 * This function is still BUGGY!
 * @param btn the SmallButton object (and state).
 * @param tDelay the time until auto-repeat starts. This is typically longer than the repeat rate.
 * @param tRepeat the repeat cycle time.
 * @param tNow the current time.
 * @return true, if auto-repeat (and button-held) happened with this sample, false otherwise.
 */
bool smallButtonSampleAutoRepeat (SmallButton *btn, Uint8 tDelay, Uint8 tRepeat, Uint8 tNow);

/** One-shot query if smallButton was just pressed.
 * @param btn the SmallButton object (and state).
 * @return true, if the button was pressed and this function NOT called since. False otherwise.
 */
inline static bool smallButtonPressedEdge (SmallButton *btn) {
	if (btn->state & SMALLBUTTON_FLAG_PRESSED) {
		btn->state &= ~SMALLBUTTON_FLAG_PRESSED;
		return true;
	}
	else return false;
}

/** One-shot query if smallButton was just released.
 * @param btn the SmallButton object (and state).
 * @return true, if the button was released and this function NOT called since. False otherwise.
 */
inline static bool smallButtonReleasedEdge (SmallButton *btn) {
	if (btn->state & SMALLBUTTON_FLAG_RELEASED) {
		btn->state &= ~SMALLBUTTON_FLAG_RELEASED;
		return true;
	}
	else return false;
}

/** One-shot query if smallButton was just held down.
 * @param btn the SmallButton object (and state).
 * @return true, if the button was held down and this function NOT called since. False otherwise.
 */
inline static bool smallButtonHeldEdge (SmallButton *btn) {
	if (btn->state & SMALLBUTTON_FLAG_HELD) {
		btn->state &= ~SMALLBUTTON_FLAG_HELD;
		return true;
	}
	else return false;
}

/** One-shot query if smallButton has just generated an auto-repeat. The first auto-repeat is also a held-down event which
 * is not canceled or influenced otherwise by this function.
 * @param btn the SmallButton object (and state).
 * @return true, if the button repeated recently and this function wasn't called since. False otherwise.
 */
inline static bool smallButtonRepeatedEdge (SmallButton *btn) {
	if (btn->state & SMALLBUTTON_FLAG_REPEATED) {
		btn->state &= ~SMALLBUTTON_FLAG_REPEATED;
		return true;
	}
	else return false;
}

/** Performs full-featured button feeding and returns an action, if available. This is an expensive function in terms of
 * code size. Event-oriented paradigm. This function returns the following actions only:
 *   BUTTON_ACTION_PRESSED,
 *   BUTTON_ACTION_RELEASED,
 *   BUTTON_ACTION_HELD,
 *   BUTTON_ACTION_REPEATED,
 *   BUTTON_ACTION_IDLE
 * @param btn the SmallButton object (and state).
 * @param allTimings deadTime, delay and auto-repeat time put into one 32bit integer in this order
 * @param tNow the current time
 * @param value the sample value of the button.
 * @return the next available action of the button or BUTTON_ACTION_IDLE if none available.
 */
ButtonAction smallButtonActionAutoRepeat (SmallButton *btn, Uint32 allTimings, Uint8 tNow, bool value);

#endif

