#include <ecMotors.h>
#include <int32Math.h>
#include <int64Math.h>
#include <simpleMath.h>

void ecSpinUpMaxonEc14A(DdsTuning256 *current, const DdsTuning256 *target, unsigned tune1Hz, unsigned fSample) {
	enum {
		HZ1000_OFF	=500,
		HZ1000_START	=2*1000,		DELTA_W_START	=4,
		HZ1000_LO	=3*1000,		DELTA_W_LO	=40,
		HZ1000_HI	=40*1000,		DELTA_W_HI	=30,	// 50
		HZ1000_MAX	=300*1000,
	 };

	unsigned deltaWMax = 0;
	const unsigned wHz1000 = current->w * 100 / (tune1Hz/10);
	if (wHz1000 >= HZ1000_MAX) ;
	else if (wHz1000 >= HZ1000_HI) {
		current->a256 = 256;
		deltaWMax = DELTA_W_HI;
	}
	else if (wHz1000 >= HZ1000_LO) {
		current->a256 = 256;
		deltaWMax = DELTA_W_LO;
	}
	else if (wHz1000 >= HZ1000_START) {	// w < HZ_LO
		//dds->amplitude256 = (dds->w - HZ_START*hpxTune1Hz()) * 256 / (hpxTune1Hz()*(HZ_LO-HZ_START));
		current->a256 = 256;
		deltaWMax = DELTA_W_START;
	}
	else {	// w < HZ_OFF
		current->a256 = 0;
		deltaWMax = DELTA_W_START;
	}

	const int dW = target->w - current->w;
	const int signDW = dW>=0 ? 1 : -1;

	if (dW*signDW > deltaWMax) current->w += (unsigned)(deltaWMax*signDW);
	else current->w = target->w;

	// do phase adjust here
	// do amplitude adjust here
}

void ecSpinUpMaxonEc14B(DdsTuning256 *current, const DdsTuning256 *target, unsigned tune1Hz, unsigned fPwm) {
	enum {
		// 0
					DELTA_W_OFF	=8,	A_OFF	=0,
		// off below HzStart
		HZ1000_START	=500,	DELTA_W_START	=3,	A_START	=0,	// off below this frequency
								// linear amplitude ramp here
		HZ1000_RUNNING	=4*1000,DELTA_W_RUNNING	=30,	A_RUNNING=192,
		HZ1000_HI	=80*1000,DELTA_W_HI	=30,	A_HI	=256
	 };

	unsigned deltaWMax = 0;
	const unsigned wHz1000 = current->w * 100 / (tune1Hz/10);
	if (wHz1000 > HZ1000_HI) ;
	else if (HZ1000_RUNNING <= wHz1000) {	// HZ_RUNNING <= f <= HZ_HI
		current->a256 = int32LinearInterpolate(
			HZ1000_RUNNING, A_RUNNING*target->a256/256,
			HZ1000_HI,A_HI*target->a256/256,
			wHz1000
		);
		deltaWMax = DELTA_W_RUNNING;
	}
	else if (HZ1000_START <= wHz1000) {	// HZ_START <= f < HZ_RUNNING
		current->a256 = int32LinearInterpolate(
			HZ1000_START,A_START*target->a256/256,
			HZ1000_RUNNING,A_RUNNING*target->a256/256,
			wHz1000
		);
		deltaWMax = DELTA_W_START;
	}
	else {					// w < HZ_START
		deltaWMax = DELTA_W_OFF;
		current->a256 = A_OFF;
	}

	const int dW = target->w - current->w;
	const int signDW = dW>=0 ? 1 : -1;

	if (dW*signDW > deltaWMax) current->w += (unsigned)(deltaWMax*signDW);
	else current->w = target->w;

	// do phase adjust here
	// do amplitude adjust here
}


void ecSpinUpMaxonEc14C(DdsTuning256 *current, const DdsTuning256 *target, unsigned tune1Hz, unsigned fPwm) {
	enum {
		// 0
						DELTA_W_OFF	=10,	A_OFF	=0,
		// off below HzStart
		HZ1000_START	=1*1000,	DELTA_W_START	=4,	A_START	=135,	// off below this frequency
								// linear amplitude ramp here
		HZ1000_RUNNING	=6*1000,	DELTA_W_RUNNING	=20,	A_RUNNING=256,
		HZ1000_HI	=80*1000,	DELTA_W_HI	=30,	A_HI	=256,
	 };

	unsigned deltaWMax = 0;
	unsigned deltaPMax = (1lu<<31)/fPwm/500;	// 1/2 revolution in ? seconds
	const unsigned wHz1000 = current->w * 100 / (tune1Hz/10);
	if (wHz1000 > HZ1000_HI) ;
	else if (HZ1000_RUNNING <= wHz1000) {	// HZ_RUNNING <= f <= HZ_HI
		current->a256 = int32LinearInterpolateFast(
			HZ1000_RUNNING, A_RUNNING*target->a256/256,
			HZ1000_HI,A_HI*target->a256/256,
			wHz1000
		);
		deltaWMax = DELTA_W_RUNNING;
		deltaPMax = (1lu<<31)/fPwm/180;	// 1/2 revolution in ? seconds
	}
	else if (HZ1000_START <= wHz1000) {	// HZ_START <= f < HZ_RUNNING
		current->a256 = int32LinearInterpolateFast(
			HZ1000_START,A_START*target->a256/256,
			HZ1000_RUNNING,A_RUNNING*target->a256/256,
			wHz1000
		);
		deltaWMax = DELTA_W_START;
	}
	else {					// w < HZ_START
		deltaWMax = DELTA_W_OFF;
		current->a256 = A_OFF;
	}

	current->w = limitMovement(current->w,target->w,deltaWMax);
	current->p = limitMovement(current->p,target->p,deltaPMax);
}

void ecSpinUpFaulhaber1509(DdsTuning256 *current, const DdsTuning256 *target, unsigned tune1Hz, unsigned fPwm) {
	enum {
		// 0
						DELTA_W_OFF	=10,	A_OFF	=0,
		// off below HzStart
		HZ1000_START	=300,		DELTA_W_START	=2,	A_START	=256,	// off below this frequency
								// linear amplitude ramp here
		HZ1000_RUNNING	=4*1000,	DELTA_W_RUNNING	=10,	A_RUNNING=256,
		HZ1000_HI	=80*1000,	DELTA_W_HI	=30,	A_HI	=256,
	 };

	unsigned deltaWMax = 0;
	unsigned deltaPMax = (1lu<<31)/fPwm/500;	// 1/2 revolution in ? seconds
	const unsigned wHz1000 = current->w * 100 / (tune1Hz/10);
	if (wHz1000 > HZ1000_HI) ;
	else if (HZ1000_RUNNING <= wHz1000) {	// HZ_RUNNING <= f <= HZ_HI
		current->a256 = int32LinearInterpolateFast(
			HZ1000_RUNNING, A_RUNNING*target->a256/256,
			HZ1000_HI,A_HI*target->a256/256,
			wHz1000
		);
		deltaWMax = DELTA_W_RUNNING;
		deltaPMax = (1lu<<31)/fPwm/180;	// 1/2 revolution in ? seconds
	}
	else if (HZ1000_START <= wHz1000) {	// HZ_START <= f < HZ_RUNNING
		current->a256 = int32LinearInterpolateFast(
			HZ1000_START,A_START*target->a256/256,
			HZ1000_RUNNING,A_RUNNING*target->a256/256,
			wHz1000
		);
		deltaWMax = DELTA_W_START;
	}
	else {					// w < HZ_START
		deltaWMax = DELTA_W_OFF;
		current->a256 = A_OFF;
	}

	current->w = limitMovement(current->w,target->w,deltaWMax);
	current->p = limitMovement(current->p,target->p,deltaPMax);
}

void ecSpinUpFaulhaber1226(DdsTuning256 *current, const DdsTuning256 *target, unsigned tune1Hz, unsigned fPwm) {
	enum {
		// 0
						DELTA_W_OFF	=10,	A_OFF	=0,
		// off below HzStart
		HZ1000_START	=10,		DELTA_W_START	=30,	A_START	=128,	// off below this frequency
								// linear amplitude ramp here
		HZ1000_RUNNING	=4*1000,	DELTA_W_RUNNING	=150,	A_RUNNING=256,
		HZ1000_HI	=1000*1000,	DELTA_W_HI	=30,	A_HI	=256,
	 };

	unsigned deltaWMax = 0;
	unsigned deltaPMax = (1lu<<31)/fPwm/500;	// 1/2 revolution in ? seconds
	const unsigned wHz1000 = current->w * 100 / (tune1Hz/10);
	if (wHz1000 > HZ1000_HI) ;
	else if (HZ1000_RUNNING <= wHz1000) {	// HZ_RUNNING <= f <= HZ_HI
		current->a256 = int32LinearInterpolateFast(
			HZ1000_RUNNING, A_RUNNING*target->a256/256,
			HZ1000_HI,A_HI*target->a256/256,
			wHz1000
		);
		deltaWMax = DELTA_W_RUNNING;
		deltaPMax = (1lu<<31)/fPwm/180;	// 1/2 revolution in ? seconds
	}
	else if (HZ1000_START <= wHz1000) {	// HZ_START <= f < HZ_RUNNING
		current->a256 = int32LinearInterpolateFast(
			HZ1000_START,A_START*target->a256/256,
			HZ1000_RUNNING,A_RUNNING*target->a256/256,
			wHz1000
		);
		deltaWMax = DELTA_W_START;
	}
	else {					// w < HZ_START
		deltaWMax = DELTA_W_OFF;
		current->a256 = A_OFF;
	}

	current->w = limitMovement(current->w,target->w,deltaWMax);
	current->p = limitMovement(current->p,target->p,deltaPMax);
}

void ecSpinUpFaulhaber1226Soft(DdsTuning256 *current, const DdsTuning256 *target, unsigned tune1Hz, unsigned fPwm) {
	enum {
		// 0
						DELTA_W_OFF	=10,	A_OFF	=0,
		// off below HzStart
		HZ1000_START	=10,		DELTA_W_START	=30,	A_START	=128,	// off below this frequency
								// linear amplitude ramp here
		HZ1000_RUNNING	=4*1000,	DELTA_W_RUNNING	=50,	A_RUNNING=256,
		HZ1000_HI	=1000*1000,	DELTA_W_HI	=30,	A_HI	=256,
	 };

	unsigned deltaWMax = 0;
	const unsigned deltaPMax = (unsigned)((1llu<<31)/5)/fPwm;	// 1/2 revolution in 5 seconds
	const int wHz1000 = current->w * 100 / (tune1Hz/10);
	const int wHz1000Abs = wHz1000>=0 ? wHz1000 : -wHz1000;

	if (wHz1000Abs > HZ1000_HI) ;
	else if (HZ1000_RUNNING <= wHz1000Abs) {	// HZ_RUNNING <= f <= HZ_HI
		current->a256 = int32LinearInterpolateFast(
			HZ1000_RUNNING, A_RUNNING*target->a256/256,
			HZ1000_HI,A_HI*target->a256/256,
			wHz1000Abs
		);
		deltaWMax = DELTA_W_RUNNING;
	}
	else if (HZ1000_START <= wHz1000Abs) {	// HZ_START <= f < HZ_RUNNING
		current->a256 = int32LinearInterpolateFast(
			HZ1000_START,A_START*target->a256/256,
			HZ1000_RUNNING,A_RUNNING*target->a256/256,
			wHz1000Abs
		);
		deltaWMax = DELTA_W_START;
	}
	else {					// w < HZ_START
		deltaWMax = DELTA_W_OFF;
		current->a256 = A_OFF;
	}

	current->w = limitMovement(current->w,target->w,deltaWMax);
	current->p = limitMovement(current->p,target->p,deltaPMax);
}

