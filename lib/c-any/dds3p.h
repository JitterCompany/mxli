#ifndef __dds3p_h
#define __dds3p_h

#include <stdbool.h>
#include <fifo.h>
#include <indexMark.h>
#include <dds.h>

/** @file
 * @brief This module provides functionality to generate multiple, independend 3-phase DDS signals.
 *
 * Driving EC motors or 3-phase AC motors requires 3 sine signals for each motor. Using the DDS principle allows
 * easy and precise tuning.
 */

/** Sine dds generation. The interrupt handler calculates sample PWMs quickly and updates indexMark
 */
struct Dds3p {
	DdsTuning256		tuning;		///< the currently active settings.
	IndexMark		indexMark;	///< zero crossing interpolation.
	volatile unsigned	phi;		///< phase accu.
	int			w1Hz;		///< tuning word for exactly 1Hz
	volatile int		ps[3];		///< DDS phase outputs: index into sine table.
	struct {
		int		phiUs;		///< latest sampling time
		int		polesBits;	///< 2^x output phase multiplier - useful for EC-motors.
		int		timeBits;	///< how many bits to use for time in the sine table.
	} internal;				///< don't touch this!
};

typedef struct Dds3p Dds3p;

/** DDS interrupt code. This must be called from the DDS interrupt to re-calculate phases and to update the D/A output
 * sample.
 * @param dds the DDS object to update.
 * @param tUs the time of the interrupt.
 */
void ddsInterrupt(Dds3p *dds, int tUs);
	
#endif
