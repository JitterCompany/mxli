#include <button.h>

const char* buttonActionToString (ButtonAction a) {
	switch(a) {
	case BUTTON_ACTION_IDLE:		return "idle";
	case BUTTON_ACTION_PRESSED:		return "pressed";
	case BUTTON_ACTION_HELD:		return "held";
	case BUTTON_ACTION_REPEATED:		return "repeated";
	case BUTTON_ACTION_RELEASED:		return "released";
	case BUTTON_ACTION_REPEATED_LEVEL1:	return "repeated L1";
	case BUTTON_ACTION_REPEATED_LEVEL2:	return "repeated L2";
	case BUTTON_ACTION_CLICK:		return "clicked";
	case BUTTON_ACTION_DOUBLECLICK:		return "double-clicked";
	case BUTTON_ACTION_TRIPLECLICK:		return "triple-clicked";
	case BUTTON_ACTION_LIMIT:		return "limit";
	default:				return "undefined";
	}
}

