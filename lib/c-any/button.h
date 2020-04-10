/*
  button.h 
  Copyright 2016 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __button_h
#define __button_h

#include <integers.h>
#include <stdbool.h>

/** @file
 * @brief Button action definitions. 
 *
 */

/* Events caused by button interaction. Button actions fit into 4 bits.
 */
enum ButtonAction {
	BUTTON_ACTION_IDLE,			///< nothing happened
	BUTTON_ACTION_PRESSED,			///< button went down
	BUTTON_ACTION_HELD,			///< button kept down
	BUTTON_ACTION_REPEATED,			///< button repeated on keeping it down
	BUTTON_ACTION_RELEASED,			///< button went up
	BUTTON_ACTION_REPEATED_LEVEL1,		///< button repeated faster (special button)
	BUTTON_ACTION_REPEATED_LEVEL2,		///< button repeated faster (special button)

	BUTTON_ACTION_CLICK,			///< button pressed and released
	BUTTON_ACTION_DOUBLECLICK,
	BUTTON_ACTION_TRIPLECLICK,
	BUTTON_ACTION_LIMIT,			///< the number of action codes = the first number unused by action codes (<16).
};

typedef Uint8 ButtonAction;

/** Converts a ButtonAction into a human-readable string.
 * @param a the action
 * @return a static string (re-entrant function).
 */
const char* buttonActionToString (ButtonAction a);

#endif

