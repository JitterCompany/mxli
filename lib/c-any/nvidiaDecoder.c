
#include <nvidiaDecoder.h>
#include <macros.h>

typedef enum {
	SCAN,		///< scanning for rising edge
	RE,		///< rising edge detected
	// LO,		///< left open detected
	SY,		///< 24us sync pulse detected
	LC_,		///< part 1 of left close detected
	// LC,		///< left close detected
	RC_,		///< part 1 of right close
	// RC,		///< right close detected
	RO_,		///< part 1 of right open detected
	// RO,		///< right open detected
} State;

volatile NvidiaPhase	phase;		///< shutter glasses state.
volatile int		lockTime;	///< time of last valid sync of any command.
volatile int		syncTime;	///< start of pulse.
volatile State		state;

enum {
	T_24		= 24,
	T_44		= 44,
	T_69		= 69,
	T_102		= 102,
	T_142		= 142,

	T_TIMEOUT	= 200,

	//T_TOL		= 3,
	T_TOL		= 8,
};

bool roughly(int t1, int t0) {
	return
		t1-(t0-T_TOL)>=0
		&& t1-(t0+T_TOL) <=0;
}

void nvidiaDecoderInit(void) {
	phase = NV_OFF;
	state = SCAN;
}

NvidiaPhase nvidiaDecoderEdge(bool level, int tUs) {
	switch(state) {
	
	case SCAN:
		if (level) {
			state = RE;
			syncTime = tUs;
		}
		break;
	case RE:
		if (!level) {
			if (roughly(tUs,syncTime+T_24)) state = SY;
			else if (roughly(tUs,syncTime+T_44)) {
				// left open
				phase = NV_L;
				state = SCAN;
				lockTime = syncTime;
			}
			else state = SCAN;	// sync error
		}
		else state = SCAN;		// sync error
		break;
	case SY:
		if (level) {
			if (roughly(tUs,syncTime+T_44)) state = LC_;
			else if (roughly(tUs,syncTime+T_69)) state = RO_;
			else if (roughly(tUs,syncTime+T_102)) state = RC_;
			else state = SCAN;	// sync error
		}
		else state = SCAN;		// sync error
		break;
	case LC_:
		if (!level && roughly(tUs,syncTime+T_69)) {
			phase = NV_OFF;	// left close
			state = SCAN;
			lockTime = syncTime;
		}
		else state = SCAN;
		break;
	case RO_:
		if (!level && roughly(tUs,syncTime+T_102)) {
			phase = NV_R;	// right open
			state = SCAN;
			lockTime = syncTime;
		}
		else state = SCAN;
		break;
	case RC_:
		if (!level && roughly(tUs,syncTime+T_142)) {
			phase = NV_OFF;	// right close
			state = SCAN;
			lockTime = syncTime;
		}
		else state = SCAN;
		break;
	}

	return phase;
}

NvidiaPhase nvidiaDecoderIdle(int tUs) {
	if (syncTime+T_TIMEOUT < tUs) {
		state = SCAN;
		phase = NV_OFF;
		syncTime = tUs;
	}
	return phase;
}

NvidiaPhase nvidiaDecoderGetPhase(void) {
	return phase;
}

